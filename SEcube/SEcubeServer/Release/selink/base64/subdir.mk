################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../selink/base64/base64.cpp 

C_SRCS += \
../selink/base64/cdecode.c \
../selink/base64/cencode.c 

OBJS += \
./selink/base64/base64.o \
./selink/base64/cdecode.o \
./selink/base64/cencode.o 

CPP_DEPS += \
./selink/base64/base64.d 

C_DEPS += \
./selink/base64/cdecode.d \
./selink/base64/cencode.d 


# Each subdirectory must supply rules for building sources it contributes
selink/base64/%.o: ../selink/base64/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -DSQLITE_TEMP_STORE=3 -std=c++17 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

selink/base64/%.o: ../selink/base64/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -DSQLITE_TEMP_STORE=3 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


