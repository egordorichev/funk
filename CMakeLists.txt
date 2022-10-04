cmake_minimum_required(VERSION 3.23)
project(funk C)

set(CMAKE_C_STANDARD 99)

include_directories(src/)
add_library(funk src/funk.c src/funk_std.c)
add_executable(funk_cli src/main.c)

target_link_libraries(funk_cli funk)
set_target_properties(funk_cli PROPERTIES OUTPUT_NAME funk)