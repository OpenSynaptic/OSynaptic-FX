set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR xtensa)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Espressif Xtensa LX6/LX7 toolchain (ESP32, ESP32-S2, ESP32-S3).
# Install: https://github.com/espressif/crosstool-NG or via IDF setup.
# The compiler binary name differs between Espressif toolchain versions:
#   Older: xtensa-esp32-elf-gcc
#   IDF 5+: xtensa-esp32-elf-gcc (same, but in ~/.espressif/tools/)
set(CMAKE_C_COMPILER xtensa-esp32-elf-gcc)
set(CMAKE_AR xtensa-esp32-elf-ar)
set(CMAKE_RANLIB xtensa-esp32-elf-ranlib)

# Target: ESP32 (LX6, 240 MHz, dual-core)
# For ESP32-S3 (LX7): change -mcpu=esp32 to -mcpu=esp32s3
set(CMAKE_C_FLAGS_INIT "-mlongcalls -mtext-section-literals -ffreestanding")
