################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../sources/L1/Crypto\ Libraries/aes256.cpp 

C_SRCS += \
../sources/L1/Crypto\ Libraries/pbkdf2.c \
../sources/L1/Crypto\ Libraries/sha256.c 

OBJS += \
./sources/L1/Crypto\ Libraries/aes256.o \
./sources/L1/Crypto\ Libraries/pbkdf2.o \
./sources/L1/Crypto\ Libraries/sha256.o 

CPP_DEPS += \
./sources/L1/Crypto\ Libraries/aes256.d 

C_DEPS += \
./sources/L1/Crypto\ Libraries/pbkdf2.d \
./sources/L1/Crypto\ Libraries/sha256.d 


# Each subdirectory must supply rules for building sources it contributes
sources/L1/Crypto\ Libraries/aes256.o: ../sources/L1/Crypto\ Libraries/aes256.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -DSQLITE_TEMP_STORE=3 -std=c++17 -MMD -MP -MF"sources/L1/Crypto Libraries/aes256.d" -MT"sources/L1/Crypto\ Libraries/aes256.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

sources/L1/Crypto\ Libraries/pbkdf2.o: ../sources/L1/Crypto\ Libraries/pbkdf2.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -DSQLITE_TEMP_STORE=3 -MMD -MP -MF"sources/L1/Crypto Libraries/pbkdf2.d" -MT"sources/L1/Crypto\ Libraries/pbkdf2.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

sources/L1/Crypto\ Libraries/sha256.o: ../sources/L1/Crypto\ Libraries/sha256.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -DSQLITE_TEMP_STORE=3 -MMD -MP -MF"sources/L1/Crypto Libraries/sha256.d" -MT"sources/L1/Crypto\ Libraries/sha256.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


