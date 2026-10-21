################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/app_ota.c \
../Core/Src/button.c \
../Core/Src/dht11.c \
../Core/Src/ds3231.c \
../Core/Src/dwt.c \
../Core/Src/flash_operations.c \
../Core/Src/i2c2.c \
../Core/Src/lcd.c \
../Core/Src/main.c \
../Core/Src/mpu6050.c \
../Core/Src/ota_tcp_server.c \
../Core/Src/stm32f1xx_hal_msp.c \
../Core/Src/stm32f1xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f1xx.c \
../Core/Src/tasks.c \
../Core/Src/timer2.c \
../Core/Src/timer3.c \
../Core/Src/usart1.c \
../Core/Src/utils.c \
../Core/Src/w25q64.c 

OBJS += \
./Core/Src/app_ota.o \
./Core/Src/button.o \
./Core/Src/dht11.o \
./Core/Src/ds3231.o \
./Core/Src/dwt.o \
./Core/Src/flash_operations.o \
./Core/Src/i2c2.o \
./Core/Src/lcd.o \
./Core/Src/main.o \
./Core/Src/mpu6050.o \
./Core/Src/ota_tcp_server.o \
./Core/Src/stm32f1xx_hal_msp.o \
./Core/Src/stm32f1xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f1xx.o \
./Core/Src/tasks.o \
./Core/Src/timer2.o \
./Core/Src/timer3.o \
./Core/Src/usart1.o \
./Core/Src/utils.o \
./Core/Src/w25q64.o 

C_DEPS += \
./Core/Src/app_ota.d \
./Core/Src/button.d \
./Core/Src/dht11.d \
./Core/Src/ds3231.d \
./Core/Src/dwt.d \
./Core/Src/flash_operations.d \
./Core/Src/i2c2.d \
./Core/Src/lcd.d \
./Core/Src/main.d \
./Core/Src/mpu6050.d \
./Core/Src/ota_tcp_server.d \
./Core/Src/stm32f1xx_hal_msp.d \
./Core/Src/stm32f1xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f1xx.d \
./Core/Src/tasks.d \
./Core/Src/timer2.d \
./Core/Src/timer3.d \
./Core/Src/usart1.d \
./Core/Src/utils.d \
./Core/Src/w25q64.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I"D:/Embedded_system/STM32/projects/STM32_OTA_Bootloader_W5500/STM32_Application_v2.0/Drivers/Ethernet_W5500/W5500" -I"D:/Embedded_system/STM32/projects/STM32_OTA_Bootloader_W5500/STM32_Application_v2.0/Drivers/Ethernet_W5500/DNS" -I"D:/Embedded_system/STM32/projects/STM32_OTA_Bootloader_W5500/STM32_Application_v2.0/Drivers/Ethernet_W5500/DHCP" -I"D:/Embedded_system/STM32/projects/STM32_OTA_Bootloader_W5500/STM32_Application_v2.0/Drivers/Ethernet_W5500" -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/app_ota.cyclo ./Core/Src/app_ota.d ./Core/Src/app_ota.o ./Core/Src/app_ota.su ./Core/Src/button.cyclo ./Core/Src/button.d ./Core/Src/button.o ./Core/Src/button.su ./Core/Src/dht11.cyclo ./Core/Src/dht11.d ./Core/Src/dht11.o ./Core/Src/dht11.su ./Core/Src/ds3231.cyclo ./Core/Src/ds3231.d ./Core/Src/ds3231.o ./Core/Src/ds3231.su ./Core/Src/dwt.cyclo ./Core/Src/dwt.d ./Core/Src/dwt.o ./Core/Src/dwt.su ./Core/Src/flash_operations.cyclo ./Core/Src/flash_operations.d ./Core/Src/flash_operations.o ./Core/Src/flash_operations.su ./Core/Src/i2c2.cyclo ./Core/Src/i2c2.d ./Core/Src/i2c2.o ./Core/Src/i2c2.su ./Core/Src/lcd.cyclo ./Core/Src/lcd.d ./Core/Src/lcd.o ./Core/Src/lcd.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/mpu6050.cyclo ./Core/Src/mpu6050.d ./Core/Src/mpu6050.o ./Core/Src/mpu6050.su ./Core/Src/ota_tcp_server.cyclo ./Core/Src/ota_tcp_server.d ./Core/Src/ota_tcp_server.o ./Core/Src/ota_tcp_server.su ./Core/Src/stm32f1xx_hal_msp.cyclo ./Core/Src/stm32f1xx_hal_msp.d ./Core/Src/stm32f1xx_hal_msp.o ./Core/Src/stm32f1xx_hal_msp.su ./Core/Src/stm32f1xx_it.cyclo ./Core/Src/stm32f1xx_it.d ./Core/Src/stm32f1xx_it.o ./Core/Src/stm32f1xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f1xx.cyclo ./Core/Src/system_stm32f1xx.d ./Core/Src/system_stm32f1xx.o ./Core/Src/system_stm32f1xx.su ./Core/Src/tasks.cyclo ./Core/Src/tasks.d ./Core/Src/tasks.o ./Core/Src/tasks.su ./Core/Src/timer2.cyclo ./Core/Src/timer2.d ./Core/Src/timer2.o ./Core/Src/timer2.su ./Core/Src/timer3.cyclo ./Core/Src/timer3.d ./Core/Src/timer3.o ./Core/Src/timer3.su ./Core/Src/usart1.cyclo ./Core/Src/usart1.d ./Core/Src/usart1.o ./Core/Src/usart1.su ./Core/Src/utils.cyclo ./Core/Src/utils.d ./Core/Src/utils.o ./Core/Src/utils.su ./Core/Src/w25q64.cyclo ./Core/Src/w25q64.d ./Core/Src/w25q64.o ./Core/Src/w25q64.su

.PHONY: clean-Core-2f-Src

