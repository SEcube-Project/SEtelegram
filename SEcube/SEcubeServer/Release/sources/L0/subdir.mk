################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../sources/L0/L0.cpp \
../sources/L0/L0_commodities.cpp \
../sources/L0/L0_communication.cpp \
../sources/L0/L0_provision.cpp 

OBJS += \
./sources/L0/L0.o \
./sources/L0/L0_commodities.o \
./sources/L0/L0_communication.o \
./sources/L0/L0_provision.o 

CPP_DEPS += \
./sources/L0/L0.d \
./sources/L0/L0_commodities.d \
./sources/L0/L0_communication.d \
./sources/L0/L0_provision.d 


# Each subdirectory must supply rules for building sources it contributes
sources/L0/%.o: ../sources/L0/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -DSQLITE_TEMP_STORE=3 -std=c++17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


