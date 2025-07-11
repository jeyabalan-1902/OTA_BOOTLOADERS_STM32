################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../BOOTLOADER_F446RET6/Core/Startup/startup_stm32f446retx.s 

OBJS += \
./BOOTLOADER_F446RET6/Core/Startup/startup_stm32f446retx.o 

S_DEPS += \
./BOOTLOADER_F446RET6/Core/Startup/startup_stm32f446retx.d 


# Each subdirectory must supply rules for building sources it contributes
BOOTLOADER_F446RET6/Core/Startup/%.o: ../BOOTLOADER_F446RET6/Core/Startup/%.s BOOTLOADER_F446RET6/Core/Startup/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m4 -g3 -DDEBUG -c -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

clean: clean-BOOTLOADER_F446RET6-2f-Core-2f-Startup

clean-BOOTLOADER_F446RET6-2f-Core-2f-Startup:
	-$(RM) ./BOOTLOADER_F446RET6/Core/Startup/startup_stm32f446retx.d ./BOOTLOADER_F446RET6/Core/Startup/startup_stm32f446retx.o

.PHONY: clean-BOOTLOADER_F446RET6-2f-Core-2f-Startup

