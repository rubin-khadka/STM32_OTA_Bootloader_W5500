################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/Ethernet_W5500/socket.c \
../Drivers/Ethernet_W5500/wizchip_conf.c \
../Drivers/Ethernet_W5500/wizchip_port.c 

OBJS += \
./Drivers/Ethernet_W5500/socket.o \
./Drivers/Ethernet_W5500/wizchip_conf.o \
./Drivers/Ethernet_W5500/wizchip_port.o 

C_DEPS += \
./Drivers/Ethernet_W5500/socket.d \
./Drivers/Ethernet_W5500/wizchip_conf.d \
./Drivers/Ethernet_W5500/wizchip_port.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/Ethernet_W5500/%.o Drivers/Ethernet_W5500/%.su Drivers/Ethernet_W5500/%.cyclo: ../Drivers/Ethernet_W5500/%.c Drivers/Ethernet_W5500/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I"D:/Embedded_system/STM32/projects/STM32_OTA_Bootloader_W5500/STM32_Application/Drivers/Ethernet_W5500" -I"D:/Embedded_system/STM32/projects/STM32_OTA_Bootloader_W5500/STM32_Application/Drivers/Ethernet_W5500/DHCP" -I"D:/Embedded_system/STM32/projects/STM32_OTA_Bootloader_W5500/STM32_Application/Drivers/Ethernet_W5500/DNS" -I"D:/Embedded_system/STM32/projects/STM32_OTA_Bootloader_W5500/STM32_Application/Drivers/Ethernet_W5500/W5500" -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Drivers-2f-Ethernet_W5500

clean-Drivers-2f-Ethernet_W5500:
	-$(RM) ./Drivers/Ethernet_W5500/socket.cyclo ./Drivers/Ethernet_W5500/socket.d ./Drivers/Ethernet_W5500/socket.o ./Drivers/Ethernet_W5500/socket.su ./Drivers/Ethernet_W5500/wizchip_conf.cyclo ./Drivers/Ethernet_W5500/wizchip_conf.d ./Drivers/Ethernet_W5500/wizchip_conf.o ./Drivers/Ethernet_W5500/wizchip_conf.su ./Drivers/Ethernet_W5500/wizchip_port.cyclo ./Drivers/Ethernet_W5500/wizchip_port.d ./Drivers/Ethernet_W5500/wizchip_port.o ./Drivers/Ethernet_W5500/wizchip_port.su

.PHONY: clean-Drivers-2f-Ethernet_W5500

