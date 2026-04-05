set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_AR avr-ar)
set(CMAKE_RANLIB avr-ranlib)

# ATmega2560: 16 MHz, 256 KB Flash, 8 KB SRAM.
# Allows more generous presets than ATmega328P; can fit secure session path.
set(CMAKE_C_FLAGS_INIT "-mmcu=atmega2560 -Os -ffunction-sections -fdata-sections -ffreestanding")
