################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../sources/L1/L1.cpp \
../sources/L1/L1_login_logout.cpp \
../sources/L1/L1_security.cpp \
../sources/L1/L1_sekey.cpp 

OBJS += \
./sources/L1/L1.o \
./sources/L1/L1_login_logout.o \
./sources/L1/L1_security.o \
./sources/L1/L1_sekey.o 

CPP_DEPS += \
./sources/L1/L1.d \
./sources/L1/L1_login_logout.d \
./sources/L1/L1_security.d \
./sources/L1/L1_sekey.d 


# Each subdirectory must supply rules for building sources it contributes
sources/L1/%.o: ../sources/L1/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -DSQLITE_TEMP_STORE=3 -std=c++17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


