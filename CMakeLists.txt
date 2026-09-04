cmake_minimum_required(VERSION 3.19)

option(BUILD_PYTHON "export python module" off)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ version selection")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(DynamicVersion)

extract_version_string(
    HEADER_FILE ${CMAKE_CURRENT_LIST_DIR}/include/urlparser.h
    VERSION_PREFIX URLPARSER_
    OUTPUT_VAR PROJECT_VERSION
)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()



project(liburlparser LANGUAGES CXX VERSION ${PROJECT_VERSION})
message(STATUS "Project: ${PROJECT_NAME}@v${PROJECT_VERSION}")
# Kept for docs/cpp templates (Doxyfile.in, footer.html.in) that still
# reference @PyProject_VERSION@.
set(PyProject_VERSION ${PROJECT_VERSION})

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
# urlparser::psl::source_url() (src/urlparser.cpp) needs this. Several
# targets compile urlparser.cpp directly (the static library, the Python
# module, the test binary) rather than linking a shared build of it, so
# this is set at directory scope here rather than repeated per-target.
add_compile_definitions(PUBLIC_SUFFIX_LIST_URL="${PUBLIC_SUFFIX_LIST_URL}")
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
    find_package(Python 3.10
            REQUIRED COMPONENTS Interpreter Development.Module
            OPTIONAL_COMPONENTS Development.SABIModule)
    # Import nanobind through CMake's find_package mechanism
    find_package(nanobind CONFIG REQUIRED)

    set(NB_MODULE _urlparser_py)

    # Linked STABLE_ABI needs Python >= 3.12 (nanobind's floor for the
    # in-process stable ABI). Below that, build a regular per-version
    # extension instead of reaching for split-mode (STABLE_ABI +
    # BACKEND_MODULE): split-mode ships no nanobind runtime in the wheel
    # itself and instead depends on a separate 'nanobind-backend' package
    # at import time - a package that, in practice, isn't reliably
    # available for every interpreter/platform combination our CI builds
    # (manylinux cp310 in particular), so installs can fail outright with
    # "No matching distribution found for nanobind-backend". Linking the
    # nanobind runtime straight into the extension avoids that dependency
    # entirely, at the cost of one wheel per Python minor version below 3.12.
    if(Python_VERSION VERSION_GREATER_EQUAL "3.12")
        set(NB_ABI_ARGS STABLE_ABI)
    else()
        set(NB_ABI_ARGS)
    endif()

    # We are now ready to compile the actual extension module
    nanobind_add_module(
            # Name of the extension
            ${NB_MODULE}

            ${NB_ABI_ARGS}

            # Source code goes here
            ${PROJECT_SOURCE_DIR}/src/binding/main.cpp ${SOURCES}
    )

    target_include_directories(${NB_MODULE} PRIVATE include src ${GENERATED_HEADERS_DIR})

    if(DEFINED SKBUILD)
        install(TARGETS ${NB_MODULE}
                CONFIGURATIONS Release
                LIBRARY DESTINATION
                ${PYTHON_PROJECT_NAME})
    else()
        file(COPY ${PROJECT_SOURCE_DIR}/src/${PROJECT_NAME} DESTINATION ${PYTHON_PROJECT_NAME})

        set_target_properties(${NB_MODULE} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PYTHON_PROJECT_NAME}/${PROJECT_NAME})
    endif()
endif()

if(DEFINED SKBUILD)
    RETURN()
endif()



add_library(urlparser STATIC ${SOURCES})
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



add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/docs)



