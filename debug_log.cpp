#include "debug_log.h"
#include <WebServer.h>
#include <esp_heap_caps.h>

DebugSerialLogger CamperSerial;

static const size_t PSRAM_BUFFER_SIZE = 64 * 1024; // 64 KB log buffer in PSRAM
static char *ring_buf = nullptr;
static size_t buf_capacity = 0;
static size_t ring_head = 0;
static size_t ring_count = 0;
static portMUX_TYPE log_mux = portMUX_INITIALIZER_UNLOCKED;

// Command injection buffer (for commands sent via Web-Terminal)
static const size_t CMD_BUF_SIZE = 64;
static char cmd_buf[CMD_BUF_SIZE];
static size_t cmd_head = 0;
static size_t cmd_tail = 0;
static size_t cmd_count = 0;
static portMUX_TYPE cmd_mux = portMUX_INITIALIZER_UNLOCKED;

DebugSerialLogger::DebugSerialLogger() {
}

static void ensure_buffer() {
    if (ring_buf != nullptr) return;
    ring_buf = (char *)heap_caps_malloc(PSRAM_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    if (ring_buf) {
        buf_capacity = PSRAM_BUFFER_SIZE;
    } else {
        // Fallback to internal RAM if PSRAM unavailable
        buf_capacity = 8192;
        ring_buf = (char *)heap_caps_malloc(buf_capacity, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    ring_head = 0;
    ring_count = 0;
}

void DebugSerialLogger::begin(unsigned long baud) {
    _hw_cdc.begin(baud);
    ensure_buffer();
}

void DebugSerialLogger::setTxTimeoutMs(uint32_t timeout) {
    _hw_cdc.setTxTimeoutMs(timeout);
}

static inline void put_char_internal(char c) {
    if (!ring_buf || buf_capacity == 0) return;
    ring_buf[ring_head] = c;
    ring_head = (ring_head + 1) % buf_capacity;
    if (ring_count < buf_capacity) {
        ring_count++;
    }
}

size_t DebugSerialLogger::write(uint8_t c) {
    _hw_cdc.write(c);
    ensure_buffer();
    portENTER_CRITICAL(&log_mux);
    put_char_internal((char)c);
    portEXIT_CRITICAL(&log_mux);
    return 1;
}

size_t DebugSerialLogger::write(const uint8_t *buffer, size_t size) {
    if (size == 0) return 0;
    _hw_cdc.write(buffer, size);
    ensure_buffer();
    portENTER_CRITICAL(&log_mux);
    for (size_t i = 0; i < size; i++) {
        put_char_internal((char)buffer[i]);
    }
    portEXIT_CRITICAL(&log_mux);
    return size;
}

void DebugSerialLogger::injectInput(char c) {
    portENTER_CRITICAL(&cmd_mux);
    if (cmd_count < CMD_BUF_SIZE) {
        cmd_buf[cmd_head] = c;
        cmd_head = (cmd_head + 1) % CMD_BUF_SIZE;
        cmd_count++;
    }
    portEXIT_CRITICAL(&cmd_mux);
}

void DebugSerialLogger::injectInput(const char *str) {
    if (!str) return;
    while (*str) {
        injectInput(*str++);
    }
}

int DebugSerialLogger::available() {
    int hw = _hw_cdc.available();
    if (hw > 0) return hw;
    portENTER_CRITICAL(&cmd_mux);
    int c = (int)cmd_count;
    portEXIT_CRITICAL(&cmd_mux);
    return c;
}

int DebugSerialLogger::read() {
    portENTER_CRITICAL(&cmd_mux);
    if (cmd_count > 0) {
        char c = cmd_buf[cmd_tail];
        cmd_tail = (cmd_tail + 1) % CMD_BUF_SIZE;
        cmd_count--;
        portEXIT_CRITICAL(&cmd_mux);
        return (uint8_t)c;
    }
    portEXIT_CRITICAL(&cmd_mux);
    return _hw_cdc.read();
}

int DebugSerialLogger::peek() {
    portENTER_CRITICAL(&cmd_mux);
    if (cmd_count > 0) {
        char c = cmd_buf[cmd_tail];
        portEXIT_CRITICAL(&cmd_mux);
        return (uint8_t)c;
    }
    portEXIT_CRITICAL(&cmd_mux);
    return _hw_cdc.peek();
}

void DebugSerialLogger::flush() {
    _hw_cdc.flush();
}

void DebugSerialLogger::clear() {
    portENTER_CRITICAL(&log_mux);
    ring_head = 0;
    ring_count = 0;
    portEXIT_CRITICAL(&log_mux);
}

size_t DebugSerialLogger::getLogCount() {
    portENTER_CRITICAL(&log_mux);
    size_t c = ring_count;
    portEXIT_CRITICAL(&log_mux);
    return c;
}

void DebugSerialLogger::streamToHttp(WebServer &server) {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/plain; charset=utf-8", "");

    ensure_buffer();

    size_t head, count, cap;
    char *buf;
    portENTER_CRITICAL(&log_mux);
    head = ring_head;
    count = ring_count;
    cap = buf_capacity;
    buf = ring_buf;
    portEXIT_CRITICAL(&log_mux);

    if (!buf || count == 0) {
        server.sendContent("");
        return;
    }

    const size_t CHUNK_SIZE = 1024;
    if (count < cap) {
        // Buffer has not wrapped yet: slice 0 .. count-1
        for (size_t offset = 0; offset < count; offset += CHUNK_SIZE) {
            size_t len = (count - offset < CHUNK_SIZE) ? (count - offset) : CHUNK_SIZE;
            server.sendContent((const char *)(buf + offset), len);
        }
    } else {
        // Buffer has wrapped:
        // Slice 1: head .. cap-1 (oldest data)
        for (size_t offset = head; offset < cap; offset += CHUNK_SIZE) {
            size_t len = (cap - offset < CHUNK_SIZE) ? (cap - offset) : CHUNK_SIZE;
            server.sendContent((const char *)(buf + offset), len);
        }
        // Slice 2: 0 .. head-1 (newest data)
        for (size_t offset = 0; offset < head; offset += CHUNK_SIZE) {
            size_t len = (head - offset < CHUNK_SIZE) ? (head - offset) : CHUNK_SIZE;
            server.sendContent((const char *)(buf + offset), len);
        }
    }
    server.sendContent("");
}

