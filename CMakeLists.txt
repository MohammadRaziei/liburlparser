cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ version selection")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
include(PyProject)
message(STATUS "Project: ${PyProject_NAME}@v${PyProject_VERSION}")


if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
endif()

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")


#if(MSVC)
#  # Microsoft Visual Studio compiler
#  set(MSVC_FLAGS "/O2")
#  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${MSVC_FLAGS}")
#elseif(CMAKE_COMPILER_IS_GNUCXX)
#  # GCC or Clang compiler
#  set(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} -O3")
#  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
#    # Linux
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
#  else()
#    # Other OS
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=generic")
#  endif()
#endif()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "" FORCE)

project(${PyProject_NAME} LANGUAGES CXX)


# Download the public suffix list file from the internet
set(PUBLIC_SUFFIX_LIST_URL "https://publicsuffix.org/list/public_suffix_list.dat")
set(PUBLIC_SUFFIX_LIST_DAT "public_suffix_list.dat")
if(NOT DEFINED SKBUILD)
    set(PUBLIC_SUFFIX_LIST_DAT "${CMAKE_CURRENT_BINARY_DIR}/${PUBLIC_SUFFIX_LIST_DAT}")
endif()

# Download the public suffix list file from the internet if not already exists
if(EXISTS "${PUBLIC_SUFFIX_LIST_DAT}")
    message(STATUS "[DATA] ${PUBLIC_SUFFIX_LIST_DAT} is found.")
else()
    message(WARNING "[DATA] ${PUBLIC_SUFFIX_LIST_DAT} is not found. Trying to download it.")
    file(DOWNLOAD "${PUBLIC_SUFFIX_LIST_URL}" ${PUBLIC_SUFFIX_LIST_DAT})
    if(EXISTS "${PUBLIC_SUFFIX_LIST_DAT}")
        message(STATUS "[DATA] ${PUBLIC_SUFFIX_LIST_DAT} is downloaded.")
    else()
        message(FATAL_ERROR "[DATA] Failed to download ${PUBLIC_SUFFIX_LIST_DAT}.")
    endif()
endif()

file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)


if(DEFINED SKBUILD)
    # Try to import all Python components potentially needed by nanobind
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

    target_sources(_core PRIVATE ${SOURCES})
    target_include_directories(_core PRIVATE include src)
    target_compile_definitions(_core PRIVATE VERSION_INFO=${SKBUILD_PROJECT_VERSION})
    target_compile_definitions(_core PRIVATE
            DONT_INIT_PSL
            PUBLIC_SUFFIX_LIST_URL="${PUBLIC_SUFFIX_LIST_URL}"
            PUBLIC_SUFFIX_LIST_DAT="${PUBLIC_SUFFIX_LIST_DAT}"
    )

    install(TARGETS _core
            CONFIGURATIONS Release
            LIBRARY DESTINATION
            ${SKBUILD_PROJECT_NAME})
    install (FILES ${PUBLIC_SUFFIX_LIST_DAT}
             DESTINATION ${SKBUILD_PROJECT_NAME})

    RETURN()
endif()


add_library(urlparser SHARED ${SOURCES})
add_library(url::base ALIAS urlparser)
target_compile_definitions(urlparser PRIVATE
        PUBLIC_SUFFIX_LIST_DAT="${PUBLIC_SUFFIX_LIST_DAT}"
)

target_include_directories(urlparser
        PUBLIC
            ${PROJECT_SOURCE_DIR}/include
        PRIVATE
            ${PROJECT_SOURCE_DIR}/src)


add_executable(example examples/main.cpp)
target_link_libraries(example PRIVATE url::base)

install(TARGETS urlparser
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include
)



enable_testing()
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/googletest)
file(GLOB TEST_SOURCES ${PROJECT_SOURCE_DIR}/tests/cpp/*.cpp)

add_executable(test_liburlparser ${TEST_SOURCES})
target_link_libraries(test_liburlparser PRIVATE url::base gtest gtest_main pthread)
target_include_directories(test_liburlparser PRIVATE ${PROJECT_SOURCE_DIR}/tests/cpp)
add_test(NAME unitTests COMMAND test_liburlparser)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} -V DEPENDS test_liburlparser)



# Find Doxygen package
find_package(Doxygen)
message(STATUS "Doxygen_FOUND : ${Doxygen_FOUND}")
# Find Doxygen package
if (Doxygen_FOUND)
    # Configure Doxyfile.in template
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

    # Add target to generate documentation
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating Doxygen documentation"
        VERBATIM
    )
endif ()




