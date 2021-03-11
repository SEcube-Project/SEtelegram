################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../sources/L1/L1\ Base/L1_base.cpp 

OBJS += \
./sources/L1/L1\ Base/L1_base.o 

CPP_DEPS += \
./sources/L1/L1\ Base/L1_base.d 


# Each subdirectory must supply rules for building sources it contributes
sources/L1/L1\ Base/L1_base.o: ../sources/L1/L1\ Base/L1_base.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -DSQLITE_TEMP_STORE=3 -std=c++17 -MMD -MP -MF"sources/L1/L1 Base/L1_base.d" -MT"sources/L1/L1\ Base/L1_base.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


