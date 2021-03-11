################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../sekey/SEkey.cpp 

OBJS += \
./sekey/SEkey.o 

CPP_DEPS += \
./sekey/SEkey.d 


# Each subdirectory must supply rules for building sources it contributes
sekey/%.o: ../sekey/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -DSQLITE_TEMP_STORE=3 -std=c++17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


