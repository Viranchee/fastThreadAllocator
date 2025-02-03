cmake_minimum_required(VERSION 3.5)

set(CMAKE_C_COMPILER /opt/homebrew/opt/llvm/bin/clang CACHE STRING "Clang compiler") 
set(CMAKE_CXX_COMPILER /opt/homebrew/opt/llvm/bin/clang++ CACHE STRING "Clang++ compiler")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile commands")

set(CMAKE_CXX_STANDARD 23 CACHE STRING "C++ standard to conform to")
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 23 CACHE STRING "C++ standard to conform to")
set(CMAKE_C_STANDARD_REQUIRED True)

project("BlockAllocator")

add_executable(blockAllocator main.cpp BlockAllocator.cpp)
