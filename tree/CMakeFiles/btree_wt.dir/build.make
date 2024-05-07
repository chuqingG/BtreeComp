# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

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

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/sun1017/MultiThreadBTree

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/sun1017/MultiThreadBTree

# Include any dependencies generated for this target.
include tree/CMakeFiles/btree_wt.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include tree/CMakeFiles/btree_wt.dir/compiler_depend.make

# Include the progress variables for this target.
include tree/CMakeFiles/btree_wt.dir/progress.make

# Include the compile flags for this target's objects.
include tree/CMakeFiles/btree_wt.dir/flags.make

tree/CMakeFiles/btree_wt.dir/btree_wt.cpp.o: tree/CMakeFiles/btree_wt.dir/flags.make
tree/CMakeFiles/btree_wt.dir/btree_wt.cpp.o: tree/btree_wt.cpp
tree/CMakeFiles/btree_wt.dir/btree_wt.cpp.o: tree/CMakeFiles/btree_wt.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/sun1017/MultiThreadBTree/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object tree/CMakeFiles/btree_wt.dir/btree_wt.cpp.o"
	cd /home/sun1017/MultiThreadBTree/tree && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT tree/CMakeFiles/btree_wt.dir/btree_wt.cpp.o -MF CMakeFiles/btree_wt.dir/btree_wt.cpp.o.d -o CMakeFiles/btree_wt.dir/btree_wt.cpp.o -c /home/sun1017/MultiThreadBTree/tree/btree_wt.cpp

tree/CMakeFiles/btree_wt.dir/btree_wt.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/btree_wt.dir/btree_wt.cpp.i"
	cd /home/sun1017/MultiThreadBTree/tree && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/sun1017/MultiThreadBTree/tree/btree_wt.cpp > CMakeFiles/btree_wt.dir/btree_wt.cpp.i

tree/CMakeFiles/btree_wt.dir/btree_wt.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/btree_wt.dir/btree_wt.cpp.s"
	cd /home/sun1017/MultiThreadBTree/tree && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/sun1017/MultiThreadBTree/tree/btree_wt.cpp -o CMakeFiles/btree_wt.dir/btree_wt.cpp.s

# Object files for target btree_wt
btree_wt_OBJECTS = \
"CMakeFiles/btree_wt.dir/btree_wt.cpp.o"

# External object files for target btree_wt
btree_wt_EXTERNAL_OBJECTS =

tree/libbtree_wt.a: tree/CMakeFiles/btree_wt.dir/btree_wt.cpp.o
tree/libbtree_wt.a: tree/CMakeFiles/btree_wt.dir/build.make
tree/libbtree_wt.a: tree/CMakeFiles/btree_wt.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/sun1017/MultiThreadBTree/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libbtree_wt.a"
	cd /home/sun1017/MultiThreadBTree/tree && $(CMAKE_COMMAND) -P CMakeFiles/btree_wt.dir/cmake_clean_target.cmake
	cd /home/sun1017/MultiThreadBTree/tree && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/btree_wt.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tree/CMakeFiles/btree_wt.dir/build: tree/libbtree_wt.a
.PHONY : tree/CMakeFiles/btree_wt.dir/build

tree/CMakeFiles/btree_wt.dir/clean:
	cd /home/sun1017/MultiThreadBTree/tree && $(CMAKE_COMMAND) -P CMakeFiles/btree_wt.dir/cmake_clean.cmake
.PHONY : tree/CMakeFiles/btree_wt.dir/clean

tree/CMakeFiles/btree_wt.dir/depend:
	cd /home/sun1017/MultiThreadBTree && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/sun1017/MultiThreadBTree /home/sun1017/MultiThreadBTree/tree /home/sun1017/MultiThreadBTree /home/sun1017/MultiThreadBTree/tree /home/sun1017/MultiThreadBTree/tree/CMakeFiles/btree_wt.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tree/CMakeFiles/btree_wt.dir/depend

