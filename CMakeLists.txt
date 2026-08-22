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

if (MSVC)
    add_compile_options(/EHsc)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()
if (CYGWIN OR UNIX)
    add_compile_definitions(_GNU_SOURCE)
endif()


find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(DEFINED SKBUILD)
    set(PYTHON_PROJECT_NAME "${SKBUILD_PROJECT_NAME}")
elseif(BUILD_PYTHON)
    set(PYTHON_PROJECT_NAME "${CMAKE_BINARY_DIR}")

    execute_process(
            COMMAND "${Python3_EXECUTABLE}" -m nanobind --cmake_dir
            OUTPUT_STRIP_TRAILING_WHITESPACE
            OUTPUT_VARIABLE NB_DIR
            ERROR_QUIET
            RESULT_VARIABLE NANOBIND_CHECK_RESULT
    )

    if(NOT NANOBIND_CHECK_RESULT EQUAL 0 OR NOT NB_DIR)
        message(WARNING "nanobind not found for ${Python3_EXECUTABLE}.")
        message(WARNING "Please install build requirements with: pip install -r requirements-dev.txt")
    else()
        message(STATUS "Found NanoBind at ${NB_DIR}")
        list(APPEND CMAKE_PREFIX_PATH "${NB_DIR}")
    endif()
endif()



# Locate the public suffix list file: prefer the copy already committed in
# the repo (source dir); only download into the build dir if it's missing.
set(PUBLIC_SUFFIX_LIST_URL "https://publicsuffix.org/list/public_suffix_list.dat")
set(PUBLIC_SUFFIX_LIST_DAT_NAME "public_suffix_list.dat")

if(EXISTS "${PROJECT_SOURCE_DIR}/${PUBLIC_SUFFIX_LIST_DAT_NAME}")
    set(PUBLIC_SUFFIX_LIST_DAT "${PROJECT_SOURCE_DIR}/${PUBLIC_SUFFIX_LIST_DAT_NAME}")
elseif(DEFINED SKBUILD)
    set(PUBLIC_SUFFIX_LIST_DAT "${PUBLIC_SUFFIX_LIST_DAT_NAME}")
else()
    set(PUBLIC_SUFFIX_LIST_DAT "${CMAKE_CURRENT_BINARY_DIR}/${PUBLIC_SUFFIX_LIST_DAT_NAME}")
endif()

# Download the public suffix list file from the internet if not already found
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

# Embed the PSL data file into a C++ header at CONFIGURE time (so it exists
# right after `cmake -B build`, not only after the first build). This
# removes the need to ship/locate a .dat file at runtime entirely.
include(${PROJECT_SOURCE_DIR}/cmake/EmbedFile.cmake)
set(GENERATED_HEADERS_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(PUBLIC_SUFFIX_LIST_DAT_H "${GENERATED_HEADERS_DIR}/public_suffix_list_dat.h")
embed_file_as_string(
        "${PUBLIC_SUFFIX_LIST_DAT}"
        "${PUBLIC_SUFFIX_LIST_DAT_H}"
        "templates"
        "public_suffix_list_dat"
)


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

        target_include_directories(${NB_MODULE} PRIVATE include src ${GENERATED_HEADERS_DIR})
        target_compile_definitions(${NB_MODULE} PRIVATE
                NB_MODULE_NAME=${NB_MODULE}
                PUBLIC_SUFFIX_LIST_URL="${PUBLIC_SUFFIX_LIST_URL}"
        )
    endforeach()

    if(DEFINED SKBUILD)
        install(TARGETS ${PYTHON_BIND_MODULES}
                CONFIGURATIONS Release
                LIBRARY DESTINATION
                ${PYTHON_PROJECT_NAME})
    else()
        file(COPY ${PROJECT_SOURCE_DIR}/src/${PROJECT_NAME} DESTINATION ${PYTHON_PROJECT_NAME})

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
target_include_directories(urlparser PRIVATE ${GENERATED_HEADERS_DIR})

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
find_package(Threads REQUIRED)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)

option(BUILD_BENCHMARK "Build the urlparser_benchmark/urlparser_profile targets" ON)
if(BUILD_BENCHMARK)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/benchmark)
endif()



add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/docs)



