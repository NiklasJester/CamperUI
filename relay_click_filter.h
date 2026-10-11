#pragma once
#include <stdint.h>
class RelayClickFilter {
    uint32_t last[16] = {};
    bool seen[16] = {};
public:
    constexpr bool accept(int channel, uint32_t now) {
        if (channel < 0 || channel >= 16) return false;
        if (seen[channel] && now - last[channel] < 600) return false;
        seen[channel] = true; last[channel] = now;
        return true;
    }
};