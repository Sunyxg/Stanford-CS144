# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/syx/code/labs/cs144/TCP_project/sponge

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/syx/code/labs/cs144/TCP_project/sponge/build

# Include any dependencies generated for this target.
include tests/CMakeFiles/send_ack.dir/depend.make

# Include the progress variables for this target.
include tests/CMakeFiles/send_ack.dir/progress.make

# Include the compile flags for this target's objects.
include tests/CMakeFiles/send_ack.dir/flags.make

tests/CMakeFiles/send_ack.dir/send_ack.cc.o: tests/CMakeFiles/send_ack.dir/flags.make
tests/CMakeFiles/send_ack.dir/send_ack.cc.o: ../tests/send_ack.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/syx/code/labs/cs144/TCP_project/sponge/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object tests/CMakeFiles/send_ack.dir/send_ack.cc.o"
	cd /home/syx/code/labs/cs144/TCP_project/sponge/build/tests && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/send_ack.dir/send_ack.cc.o -c /home/syx/code/labs/cs144/TCP_project/sponge/tests/send_ack.cc

tests/CMakeFiles/send_ack.dir/send_ack.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/send_ack.dir/send_ack.cc.i"
	cd /home/syx/code/labs/cs144/TCP_project/sponge/build/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/syx/code/labs/cs144/TCP_project/sponge/tests/send_ack.cc > CMakeFiles/send_ack.dir/send_ack.cc.i

tests/CMakeFiles/send_ack.dir/send_ack.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/send_ack.dir/send_ack.cc.s"
	cd /home/syx/code/labs/cs144/TCP_project/sponge/build/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/syx/code/labs/cs144/TCP_project/sponge/tests/send_ack.cc -o CMakeFiles/send_ack.dir/send_ack.cc.s

# Object files for target send_ack
send_ack_OBJECTS = \
"CMakeFiles/send_ack.dir/send_ack.cc.o"

# External object files for target send_ack
send_ack_EXTERNAL_OBJECTS =

tests/send_ack: tests/CMakeFiles/send_ack.dir/send_ack.cc.o
tests/send_ack: tests/CMakeFiles/send_ack.dir/build.make
tests/send_ack: tests/libspongechecks.a
tests/send_ack: libsponge/libsponge.a
tests/send_ack: tests/CMakeFiles/send_ack.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/syx/code/labs/cs144/TCP_project/sponge/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable send_ack"
	cd /home/syx/code/labs/cs144/TCP_project/sponge/build/tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/send_ack.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tests/CMakeFiles/send_ack.dir/build: tests/send_ack

.PHONY : tests/CMakeFiles/send_ack.dir/build

tests/CMakeFiles/send_ack.dir/clean:
	cd /home/syx/code/labs/cs144/TCP_project/sponge/build/tests && $(CMAKE_COMMAND) -P CMakeFiles/send_ack.dir/cmake_clean.cmake
.PHONY : tests/CMakeFiles/send_ack.dir/clean

tests/CMakeFiles/send_ack.dir/depend:
	cd /home/syx/code/labs/cs144/TCP_project/sponge/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/syx/code/labs/cs144/TCP_project/sponge /home/syx/code/labs/cs144/TCP_project/sponge/tests /home/syx/code/labs/cs144/TCP_project/sponge/build /home/syx/code/labs/cs144/TCP_project/sponge/build/tests /home/syx/code/labs/cs144/TCP_project/sponge/build/tests/CMakeFiles/send_ack.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tests/CMakeFiles/send_ack.dir/depend

