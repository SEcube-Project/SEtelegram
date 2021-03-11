################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../setelegram/SEtelegram.cpp 

OBJS += \
./setelegram/SEtelegram.o 

CPP_DEPS += \
./setelegram/SEtelegram.d 


# Each subdirectory must supply rules for building sources it contributes
setelegram/%.o: ../setelegram/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -DSQLITE_TEMP_STORE=3 -std=c++17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


