################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Application/User/Core/Src/cmd_parser.c \
../Application/User/Core/Src/debug_log.c \
../Application/User/Core/Src/dht11.c \
../Application/User/Core/Src/drv_flash.c \
../Application/User/Core/Src/drv_key.c \
../Application/User/Core/Src/drv_uart_dma.c \
../Application/User/Core/Src/filter.c \
../Application/User/Core/Src/modbus_service.c \
../Application/User/Core/Src/mq2.c \
../Application/User/Core/Src/ringbuffer.c \
../Application/User/Core/Src/ssd1306.c \
../Application/User/Core/Src/storage_service.c 

OBJS += \
./Application/User/Core/Src/cmd_parser.o \
./Application/User/Core/Src/debug_log.o \
./Application/User/Core/Src/dht11.o \
./Application/User/Core/Src/drv_flash.o \
./Application/User/Core/Src/drv_key.o \
./Application/User/Core/Src/drv_uart_dma.o \
./Application/User/Core/Src/filter.o \
./Application/User/Core/Src/modbus_service.o \
./Application/User/Core/Src/mq2.o \
./Application/User/Core/Src/ringbuffer.o \
./Application/User/Core/Src/ssd1306.o \
./Application/User/Core/Src/storage_service.o 

C_DEPS += \
./Application/User/Core/Src/cmd_parser.d \
./Application/User/Core/Src/debug_log.d \
./Application/User/Core/Src/dht11.d \
./Application/User/Core/Src/drv_flash.d \
./Application/User/Core/Src/drv_key.d \
./Application/User/Core/Src/drv_uart_dma.d \
./Application/User/Core/Src/filter.d \
./Application/User/Core/Src/modbus_service.d \
./Application/User/Core/Src/mq2.d \
./Application/User/Core/Src/ringbuffer.d \
./Application/User/Core/Src/ssd1306.d \
./Application/User/Core/Src/storage_service.d 


# Each subdirectory must supply rules for building sources it contributes
Application/User/Core/Src/%.o Application/User/Core/Src/%.su Application/User/Core/Src/%.cyclo: ../Application/User/Core/Src/%.c Application/User/Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../../Core/Inc -I../../Drivers/STM32F1xx_HAL_Driver/Inc -I../../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../../Drivers/CMSIS/Include -I"D:/STM32Project/FreeRTOS/a/IndustrialGateway/STM32CubeIDE/Application/User/Core/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Application-2f-User-2f-Core-2f-Src

clean-Application-2f-User-2f-Core-2f-Src:
	-$(RM) ./Application/User/Core/Src/cmd_parser.cyclo ./Application/User/Core/Src/cmd_parser.d ./Application/User/Core/Src/cmd_parser.o ./Application/User/Core/Src/cmd_parser.su ./Application/User/Core/Src/debug_log.cyclo ./Application/User/Core/Src/debug_log.d ./Application/User/Core/Src/debug_log.o ./Application/User/Core/Src/debug_log.su ./Application/User/Core/Src/dht11.cyclo ./Application/User/Core/Src/dht11.d ./Application/User/Core/Src/dht11.o ./Application/User/Core/Src/dht11.su ./Application/User/Core/Src/drv_flash.cyclo ./Application/User/Core/Src/drv_flash.d ./Application/User/Core/Src/drv_flash.o ./Application/User/Core/Src/drv_flash.su ./Application/User/Core/Src/drv_key.cyclo ./Application/User/Core/Src/drv_key.d ./Application/User/Core/Src/drv_key.o ./Application/User/Core/Src/drv_key.su ./Application/User/Core/Src/drv_uart_dma.cyclo ./Application/User/Core/Src/drv_uart_dma.d ./Application/User/Core/Src/drv_uart_dma.o ./Application/User/Core/Src/drv_uart_dma.su ./Application/User/Core/Src/filter.cyclo ./Application/User/Core/Src/filter.d ./Application/User/Core/Src/filter.o ./Application/User/Core/Src/filter.su ./Application/User/Core/Src/modbus_service.cyclo ./Application/User/Core/Src/modbus_service.d ./Application/User/Core/Src/modbus_service.o ./Application/User/Core/Src/modbus_service.su ./Application/User/Core/Src/mq2.cyclo ./Application/User/Core/Src/mq2.d ./Application/User/Core/Src/mq2.o ./Application/User/Core/Src/mq2.su ./Application/User/Core/Src/ringbuffer.cyclo ./Application/User/Core/Src/ringbuffer.d ./Application/User/Core/Src/ringbuffer.o ./Application/User/Core/Src/ringbuffer.su ./Application/User/Core/Src/ssd1306.cyclo ./Application/User/Core/Src/ssd1306.d ./Application/User/Core/Src/ssd1306.o ./Application/User/Core/Src/ssd1306.su ./Application/User/Core/Src/storage_service.cyclo ./Application/User/Core/Src/storage_service.d ./Application/User/Core/Src/storage_service.o ./Application/User/Core/Src/storage_service.su

.PHONY: clean-Application-2f-User-2f-Core-2f-Src

