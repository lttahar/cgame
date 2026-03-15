cmake_minimum_required(VERSION 3.16)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True) # Ensures a C99 compiler is found or configuration fails

project(SnakeC02 C)

find_package(SDL3 REQUIRED)

add_executable(SnakeC02 snake02.c)
target_link_libraries(SnakeC02 PRIVATE SDL3::SDL3)

target_compile_options(SnakeC02 PRIVATE -Wall -Wextra -Warray-bounds)

#add_compile_options(-Wall -Wextra -Wpedantic)

