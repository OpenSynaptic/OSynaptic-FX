set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_RANLIB arm-none-eabi-ranlib)

# Generic Cortex-M0+ (STM32C0, SAM D21, etc.)
# For RP2040-specific build use cmake/toolchains/rp2040.cmake instead.
set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-m0plus -mthumb -ffreestanding")
