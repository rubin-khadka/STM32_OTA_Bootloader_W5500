################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/bl_backup.c \
../Core/Src/bl_jump.c \
../Core/Src/bl_ota.c \
../Core/Src/crc32.c \
../Core/Src/flash_operations.c \
../Core/Src/main.c \
../Core/Src/stm32f1xx_hal_msp.c \
../Core/Src/stm32f1xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f1xx.c \
../Core/Src/usart1.c \
../Core/Src/w25q64.c 

OBJS += \
./Core/Src/bl_backup.o \
./Core/Src/bl_jump.o \
./Core/Src/bl_ota.o \
./Core/Src/crc32.o \
./Core/Src/flash_operations.o \
./Core/Src/main.o \
./Core/Src/stm32f1xx_hal_msp.o \
./Core/Src/stm32f1xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f1xx.o \
./Core/Src/usart1.o \
./Core/Src/w25q64.o 

C_DEPS += \
./Core/Src/bl_backup.d \
./Core/Src/bl_jump.d \
./Core/Src/bl_ota.d \
./Core/Src/crc32.d \
./Core/Src/flash_operations.d \
./Core/Src/main.d \
./Core/Src/stm32f1xx_hal_msp.d \
./Core/Src/stm32f1xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f1xx.d \
./Core/Src/usart1.d \
./Core/Src/w25q64.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/bl_backup.cyclo ./Core/Src/bl_backup.d ./Core/Src/bl_backup.o ./Core/Src/bl_backup.su ./Core/Src/bl_jump.cyclo ./Core/Src/bl_jump.d ./Core/Src/bl_jump.o ./Core/Src/bl_jump.su ./Core/Src/bl_ota.cyclo ./Core/Src/bl_ota.d ./Core/Src/bl_ota.o ./Core/Src/bl_ota.su ./Core/Src/crc32.cyclo ./Core/Src/crc32.d ./Core/Src/crc32.o ./Core/Src/crc32.su ./Core/Src/flash_operations.cyclo ./Core/Src/flash_operations.d ./Core/Src/flash_operations.o ./Core/Src/flash_operations.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/stm32f1xx_hal_msp.cyclo ./Core/Src/stm32f1xx_hal_msp.d ./Core/Src/stm32f1xx_hal_msp.o ./Core/Src/stm32f1xx_hal_msp.su ./Core/Src/stm32f1xx_it.cyclo ./Core/Src/stm32f1xx_it.d ./Core/Src/stm32f1xx_it.o ./Core/Src/stm32f1xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f1xx.cyclo ./Core/Src/system_stm32f1xx.d ./Core/Src/system_stm32f1xx.o ./Core/Src/system_stm32f1xx.su ./Core/Src/usart1.cyclo ./Core/Src/usart1.d ./Core/Src/usart1.o ./Core/Src/usart1.su ./Core/Src/w25q64.cyclo ./Core/Src/w25q64.d ./Core/Src/w25q64.o ./Core/Src/w25q64.su

.PHONY: clean-Core-2f-Src

