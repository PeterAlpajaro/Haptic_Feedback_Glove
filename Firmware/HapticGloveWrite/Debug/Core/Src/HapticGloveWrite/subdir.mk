################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/HapticGloveWrite/bluenrg_init.c \
../Core/Src/HapticGloveWrite/gatt_db.c \
../Core/Src/HapticGloveWrite/motor_control.c \
../Core/Src/HapticGloveWrite/sensor.c 

OBJS += \
./Core/Src/HapticGloveWrite/bluenrg_init.o \
./Core/Src/HapticGloveWrite/gatt_db.o \
./Core/Src/HapticGloveWrite/motor_control.o \
./Core/Src/HapticGloveWrite/sensor.o 

C_DEPS += \
./Core/Src/HapticGloveWrite/bluenrg_init.d \
./Core/Src/HapticGloveWrite/gatt_db.d \
./Core/Src/HapticGloveWrite/motor_control.d \
./Core/Src/HapticGloveWrite/sensor.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/HapticGloveWrite/%.o Core/Src/HapticGloveWrite/%.su Core/Src/HapticGloveWrite/%.cyclo: ../Core/Src/HapticGloveWrite/%.c Core/Src/HapticGloveWrite/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L496xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../BlueNRG-2/Target -I../Middlewares/ST/BlueNRG-2/hci/hci_tl_patterns/Basic -I../Middlewares/ST/BlueNRG-2/utils -I../Middlewares/ST/BlueNRG-2/includes -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-HapticGloveWrite

clean-Core-2f-Src-2f-HapticGloveWrite:
	-$(RM) ./Core/Src/HapticGloveWrite/bluenrg_init.cyclo ./Core/Src/HapticGloveWrite/bluenrg_init.d ./Core/Src/HapticGloveWrite/bluenrg_init.o ./Core/Src/HapticGloveWrite/bluenrg_init.su ./Core/Src/HapticGloveWrite/gatt_db.cyclo ./Core/Src/HapticGloveWrite/gatt_db.d ./Core/Src/HapticGloveWrite/gatt_db.o ./Core/Src/HapticGloveWrite/gatt_db.su ./Core/Src/HapticGloveWrite/motor_control.cyclo ./Core/Src/HapticGloveWrite/motor_control.d ./Core/Src/HapticGloveWrite/motor_control.o ./Core/Src/HapticGloveWrite/motor_control.su ./Core/Src/HapticGloveWrite/sensor.cyclo ./Core/Src/HapticGloveWrite/sensor.d ./Core/Src/HapticGloveWrite/sensor.o ./Core/Src/HapticGloveWrite/sensor.su

.PHONY: clean-Core-2f-Src-2f-HapticGloveWrite

