set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# AVR 8-bit toolchain (avr-gcc, part of AVR-GCC cross toolchain).
# Windows: install WinAVR or avr-gcc from Microchip/Atmel Studio.
# Linux/macOS: apt install gcc-avr binutils-avr
set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_AR avr-ar)
set(CMAKE_RANLIB avr-ranlib)

# ATmega328P: 8 MHz/16 MHz, 32 KB Flash, 2 KB SRAM.
# IMPORTANT: This preset uses -Os (size optimisation) and -ffunction-sections
# -fdata-sections so the linker can dead-strip unused symbols down from the
# 97 KB archive to the subset actually referenced by the sketch.
# The linked binary must remain under 32 KB Flash and ~1.3 KB used SRAM
# (leaving 256 B for stack + 256 B margin at the minimum AVR preset).
set(CMAKE_C_FLAGS_INIT "-mmcu=atmega328p -Os -ffunction-sections -fdata-sections -ffreestanding")
