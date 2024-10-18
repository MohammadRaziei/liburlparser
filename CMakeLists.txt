cmake_minimum_required(VERSION 3.19)

option(BUILD_PYTHON "export python module" off)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ version selection")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(PyProject)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()



set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "" FORCE)

project(${PyProject_NAME} LANGUAGES CXX VERSION ${PyProject_VERSION})
message(STATUS "Project: ${PROJECT_NAME}@v${PROJECT_VERSION}")

find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(DEFINED SKBUILD)
    set(PYTHON_PROJECT_NAME "${SKBUILD_PROJECT_NAME}")
elseif(BUILD_PYTHON)
    set(PYTHON_PROJECT_NAME "${CMAKE_BINARY_DIR}")

    if(NOT PYTHON_REQUIREMENT_INSTALLED)
        execute_process(
                COMMAND "${Python3_EXECUTABLE}" -m pip install
                nanobind ninja pytest-xdist pip-tools # build requirements
                OUTPUT_QUIET COMMAND_ERROR_IS_FATAL ANY
        )
        execute_process(
                COMMAND "${Python3_EXECUTABLE}" -m piptools compile --output-file ${CMAKE_CURRENT_BINARY_DIR}/requirements.txt pyproject.toml
                    --no-emit-options --quiet --no-strip-extras --extra test
                OUTPUT_QUIET COMMAND_ERROR_IS_FATAL ANY
        )
        execute_process(
                COMMAND "${Python3_EXECUTABLE}" -m pip install -r ${CMAKE_CURRENT_BINARY_DIR}/requirements.txt
                OUTPUT_QUIET COMMAND_ERROR_IS_FATAL ANY
        )
        set(PYTHON_REQUIREMENT_INSTALLED TRUE CACHE INTERNAL "Python requirements installed")
    endif()

    execute_process(
            COMMAND "${Python3_EXECUTABLE}" -m nanobind --cmake_dir
            OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE NB_DIR)

    message(STATUS "Found NanoBind at ${NB_DIR}")
    list(APPEND CMAKE_PREFIX_PATH "${NB_DIR}")
endif()



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


if(DEFINED PYTHON_PROJECT_NAME)
    # Try to import all Python components potentially needed by nanobind
    #set(Python3_FIND_STRATEGY LOCATION)
    find_package(Python 3.8
            REQUIRED COMPONENTS Interpreter Development.Module
            OPTIONAL_COMPONENTS Development.SABIModule)
    # Import nanobind through CMake's find_package mechanism
    find_package(nanobind CONFIG REQUIRED)


    set(NB_MODULE _about)
    nanobind_add_module(${NB_MODULE} STABLE_ABI NB_STATIC ${PROJECT_SOURCE_DIR}/src/binding/about.cpp)
    target_compile_definitions(${NB_MODULE} PRIVATE
            VERSION_INFO=${PROJECT_VERSION}
            NB_MODULE_NAME=${NB_MODULE})

    file(GLOB PYTHON_BIND_MODULES_PATH ${PROJECT_SOURCE_DIR}/src/binding/bind_*)
    set(PYTHON_BIND_MODULES "${NB_MODULE}")
    foreach(NB_MODULE_PATH IN LISTS PYTHON_BIND_MODULES_PATH)
        get_filename_component(NB_MODULE ${NB_MODULE_PATH} NAME)
        string(REGEX REPLACE "bind(.*)" "\\1" NB_MODULE ${NB_MODULE})
        list(APPEND PYTHON_BIND_MODULES ${NB_MODULE})
        file(GLOB NB_MODULE_SOURCES ${NB_MODULE_PATH}/*.cpp)

        message(STATUS "Found module: ${NB_MODULE}")

        # Add more commands here to process each directory as needed

        # We are now ready to compile the actual extension module
        nanobind_add_module(
                # Name of the extension
                ${NB_MODULE}

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
                ${NB_MODULE_SOURCES} ${SOURCES}
        )

        target_include_directories(${NB_MODULE} PRIVATE include src)
        target_compile_definitions(${NB_MODULE} PRIVATE NB_MODULE_NAME=${NB_MODULE})
        target_compile_definitions(${NB_MODULE} PRIVATE
                DONT_INIT_PSL
                PUBLIC_SUFFIX_LIST_URL="${PUBLIC_SUFFIX_LIST_URL}"
                PUBLIC_SUFFIX_LIST_DAT="${PUBLIC_SUFFIX_LIST_DAT}"
        )
    endforeach()

    if(DEFINED SKBUILD)
        install(TARGETS ${PYTHON_BIND_MODULES}
                CONFIGURATIONS Release
                LIBRARY DESTINATION
                ${PYTHON_PROJECT_NAME})
        install (FILES ${PUBLIC_SUFFIX_LIST_DAT}
                DESTINATION ${SKBUILD_PROJECT_NAME})
    else()
        file(COPY ${PROJECT_SOURCE_DIR}/src/${PROJECT_NAME} DESTINATION ${PYTHON_PROJECT_NAME})
        file(COPY ${PUBLIC_SUFFIX_LIST_DAT} DESTINATION ${PYTHON_PROJECT_NAME}/${PROJECT_NAME} )

        foreach(_core IN LISTS PYTHON_BIND_MODULES)
            set_target_properties(${_core} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PYTHON_PROJECT_NAME}/${PROJECT_NAME})
        endforeach()
    endif()
endif()

if(DEFINED SKBUILD)
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


if(BUILD_PYTHON)
    add_test(NAME pythonTests
            COMMAND ${Python3_EXECUTABLE} -m pytest -n auto ${PROJECT_SOURCE_DIR}/tests/python
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()



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




