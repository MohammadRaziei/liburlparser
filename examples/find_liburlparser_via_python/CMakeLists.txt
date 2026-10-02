cmake_minimum_required(VERSION 3.15)
project(liburlparser_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Ask the *installed* liburlparser Python package where its CMake config
# lives (the same pattern nanobind uses: `python -m nanobind --cmake_dir`)
# and hand that to find_package - no manual -I/-L flags, no hardcoded paths.
execute_process(
    COMMAND ${Python3_EXECUTABLE} -m liburlparser --cmake-dir
    OUTPUT_VARIABLE liburlparser_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
)

find_package(liburlparser CONFIG REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE urlparser::urlparser)

# Plain paths are available too, for build systems that don't use the target:
message(STATUS "urlparser_LIB_PATH     = ${urlparser_LIB_PATH}")
message(STATUS "urlparser_INCLUDE_PATH = ${urlparser_INCLUDE_PATH}")
