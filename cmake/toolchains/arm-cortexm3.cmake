set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_RANLIB arm-none-eabi-ranlib)

# ARM Cortex-M3 (STM32F1, LPC17xx, etc.)
set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-m3 -mthumb -ffreestanding")
