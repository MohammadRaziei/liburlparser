cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ version selection")  # or 11, 14, 17, 20
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # optional, ensure standard is supported
set(CMAKE_CXX_EXTENSIONS OFF)  # optional, keep compiler extensions off

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

project(liburlparser LANGUAGES CXX)


file(DOWNLOAD https://publicsuffix.org/list/public_suffix_list.dat public_suffix_list.dat)

file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_library(urlparser SHARED ${SOURCES})
add_library(url::base ALIAS urlparser)

target_include_directories(urlparser
        PUBLIC
            ${PROJECT_SOURCE_DIR}/include
        PRIVATE
            ${PROJECT_SOURCE_DIR}/src)

if(DEFINED SKBUILD)
    # Try to import all Python components potentially needed by nanobind
#    file(DOWNLOAD https://publicsuffix.org/list/public_suffix_list.dat ${SKBUILD_DATA_DIR}/${SKBUILD_PROJECT_NAME}/public_suffix_list.dat)

    find_package(Python 3.8
        REQUIRED COMPONENTS Interpreter Development.Module
        OPTIONAL_COMPONENTS Development.SABIModule)

    # Import nanobind through CMake's find_package mechanism
    find_package(nanobind CONFIG REQUIRED)

    # We are now ready to compile the actual extension module
    nanobind_add_module(
        # Name of the extension
        _core

        # Target the stable ABI for Python 3.12+, which reduces
        # the number of binary wheels that must be built. This
        # does nothing on older Python versions
        STABLE_ABI

        # Build libnanobind statically and merge it into the
        # extension (which itself remains a shared library)
        #
        # If your project builds multiple extensions, you can
        # replace this flag by NB_SHARED to conserve space by
        # reusing a shared libnanobind across libraries
        NB_STATIC

        # Source code goes here
        src/binding/main.cpp
    )

#    set(PYBIND11_NEWPYTHON ON)
#    find_package(pybind11 CONFIG REQUIRED)
#    pybind11_add_module(_core src/${SKBUILD_PROJECT_NAME}/main.cpp)
    target_sources(_core PRIVATE ${SOURCES})
    target_include_directories(_core PRIVATE include src)
    target_compile_definitions(_core PRIVATE VERSION_INFO=${SKBUILD_PROJECT_VERSION})
    target_compile_definitions(_core PRIVATE DONT_INIT_PSL)

    install(TARGETS _core
            CONFIGURATIONS Release
            LIBRARY DESTINATION
            ${SKBUILD_PROJECT_NAME})
    install (FILES public_suffix_list.dat
        DESTINATION ${SKBUILD_PROJECT_NAME})

    RETURN()
endif()

add_executable(example examples/main.cpp)

target_link_libraries(example PRIVATE url::base)

find_package(GTest REQUIRED)

message(STATUS "GTest_FOUND : ${GTest_FOUND}")

enable_testing()
add_executable(liburlparser_test tests/test.cpp)
target_link_libraries(liburlparser_test PRIVATE url::base gtest pthread)

add_test(NAME unitTests COMMAND liburlparser_test)


install(TARGETS urlparser
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include
)

# Find Doxygen package
find_package(Doxygen)
message(STATUS "Doxygen_FOUND : ${Doxygen_FOUND}")

# Find Doxygen package
find_package(Doxygen REQUIRED)

# Configure Doxyfile.in template
configure_file(docs/Doxyfile.in ${CMAKE_BINARY_DIR}/Doxyfile @ONLY)

# Add target to generate Doxyfile
add_custom_target(generate_doxyfile
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/Doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    COMMENT "Generating Doxyfile"
)

# Add target to generate documentation
add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating Doxygen documentation"
    VERBATIM
)




