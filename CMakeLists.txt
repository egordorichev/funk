cmake_minimum_required(VERSION 3.23)
project(funk C)

set(CMAKE_C_STANDARD 99)

include_directories(src/)
add_library(funk src/funk.c src/funk_std.c)
add_executable(funk_cli src/main.c)

target_link_libraries(funk_cli funk m)
set_target_properties(funk_cli PROPERTIES OUTPUT_NAME funk)

set_target_properties(funk_cli PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dist")
set_target_properties(funk_cli PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dist")

install(TARGETS funk_cli DESTINATION bin)