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

# Utility rule file for tidy_quiet_tcp_helpers__ethernet_header.cc.

# Include the progress variables for this target.
include CMakeFiles/tidy_quiet_tcp_helpers__ethernet_header.cc.dir/progress.make

CMakeFiles/tidy_quiet_tcp_helpers__ethernet_header.cc:
	clang-tidy -checks='*,-fuchsia-*,-hicpp-signed-bitwise,-google-build-using-namespace,-android*,-cppcoreguidelines-pro-bounds-pointer-arithmetic,-google-runtime-references,-readability-avoid-const-params-in-decls,-llvm-header-guard' -header-filter=.* -p=/home/syx/code/labs/cs144/TCP_project/sponge/build /home/syx/code/labs/cs144/TCP_project/sponge/libsponge/tcp_helpers/ethernet_header.cc 2>/dev/null

tidy_quiet_tcp_helpers__ethernet_header.cc: CMakeFiles/tidy_quiet_tcp_helpers__ethernet_header.cc
tidy_quiet_tcp_helpers__ethernet_header.cc: CMakeFiles/tidy_quiet_tcp_helpers__ethernet_header.cc.dir/build.make

.PHONY : tidy_quiet_tcp_helpers__ethernet_header.cc

# Rule to build all files generated by this target.
CMakeFiles/tidy_quiet_tcp_helpers__ethernet_header.cc.dir/build: tidy_quiet_tcp_helpers__ethernet_header.cc

.PHONY : CMakeFiles/tidy_quiet_tcp_helpers__ethernet_header.cc.dir/build

CMakeFiles/tidy_quiet_tcp_helpers__ethernet_header.cc.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/tidy_quiet_tcp_helpers__ethernet_header.cc.dir/cmake_clean.cmake
.PHONY : CMakeFiles/tidy_quiet_tcp_helpers__ethernet_header.cc.dir/clean

CMakeFiles/tidy_quiet_tcp_helpers__ethernet_header.cc.dir/depend:
	cd /home/syx/code/labs/cs144/TCP_project/sponge/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/syx/code/labs/cs144/TCP_project/sponge /home/syx/code/labs/cs144/TCP_project/sponge /home/syx/code/labs/cs144/TCP_project/sponge/build /home/syx/code/labs/cs144/TCP_project/sponge/build /home/syx/code/labs/cs144/TCP_project/sponge/build/CMakeFiles/tidy_quiet_tcp_helpers__ethernet_header.cc.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/tidy_quiet_tcp_helpers__ethernet_header.cc.dir/depend

