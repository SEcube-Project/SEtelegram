# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.17

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /snap/clion/139/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /snap/clion/139/bin/cmake/linux/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/matteo/Scrivania/SEtelegram

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/matteo/Scrivania/SEtelegram/cmake-build-debug

# Utility rule file for SEtelegram_autogen.

# Include the progress variables for this target.
include CMakeFiles/SEtelegram_autogen.dir/progress.make

CMakeFiles/SEtelegram_autogen:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/matteo/Scrivania/SEtelegram/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Automatic MOC and UIC for target SEtelegram"
	/snap/clion/139/bin/cmake/linux/bin/cmake -E cmake_autogen /home/matteo/Scrivania/SEtelegram/cmake-build-debug/CMakeFiles/SEtelegram_autogen.dir/AutogenInfo.json Debug

SEtelegram_autogen: CMakeFiles/SEtelegram_autogen
SEtelegram_autogen: CMakeFiles/SEtelegram_autogen.dir/build.make

.PHONY : SEtelegram_autogen

# Rule to build all files generated by this target.
CMakeFiles/SEtelegram_autogen.dir/build: SEtelegram_autogen

.PHONY : CMakeFiles/SEtelegram_autogen.dir/build

CMakeFiles/SEtelegram_autogen.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/SEtelegram_autogen.dir/cmake_clean.cmake
.PHONY : CMakeFiles/SEtelegram_autogen.dir/clean

CMakeFiles/SEtelegram_autogen.dir/depend:
	cd /home/matteo/Scrivania/SEtelegram/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/matteo/Scrivania/SEtelegram /home/matteo/Scrivania/SEtelegram /home/matteo/Scrivania/SEtelegram/cmake-build-debug /home/matteo/Scrivania/SEtelegram/cmake-build-debug /home/matteo/Scrivania/SEtelegram/cmake-build-debug/CMakeFiles/SEtelegram_autogen.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/SEtelegram_autogen.dir/depend

