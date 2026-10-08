@echo off
echo ===========================================
echo  CamperUI - Kompilieren ^& Exportieren (.bin)
echo ===========================================
set ARDUINO_CLI="C:\Users\njest\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"

if not exist "%ARDUINO_CLI%" (
    echo Arduino CLI nicht gefunden!
    pause
    exit /b 1
)

echo Kompiliere CamperUI...
%ARDUINO_CLI% compile --output-dir ./build -b esp32:esp32:esp32s3:CPUFreq=240,CDCOnBoot=cdc,PSRAM=opi,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB CamperUI.ino

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo FEHLER beim Kompilieren!
    pause
    exit /b %ERRORLEVEL%
)

copy /Y ".\build\CamperUI.ino.bin" ".\build\CamperUI.bin" >nul
copy /Y ".\build\CamperUI.ino.bootloader.bin" ".\build\bootloader.bin" >nul
copy /Y ".\build\CamperUI.ino.partitions.bin" ".\build\partitions.bin" >nul
if exist ".\build\CamperUI.ino.merged.bin" (
    copy /Y ".\build\CamperUI.ino.merged.bin" ".\build\CamperUI-merged.bin" >nul
)

echo.
echo ===========================================
echo  Erfolgreich exportiert nach .\build\
echo  - .\build\CamperUI.bin (fuer GitHub OTA Release)
echo  - .\build\CamperUI-merged.bin (fuer Web-Flasher)
echo ===========================================
pause
