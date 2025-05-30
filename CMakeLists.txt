cmake_minimum_required(VERSION 3.24)  # CMake version check
project(scratchpad)               # Create project "simple_example"
# enable_language(CXX)
enable_language(C)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add main.cpp file of the project root directory as a source file
set(SOURCE_FILES pzl.c)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(pzl_sim ${SOURCE_FILES})