set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_RANLIB arm-none-eabi-ranlib)

# ARM Cortex-M33 with TrustZone (STM32U5, STM32C5, LPC55Sxx, nRF9160)
# DSP extension enabled; no-secure build (NS attribute not set here).
set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard -ffreestanding")
