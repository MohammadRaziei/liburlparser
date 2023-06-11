cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)


project(liburlparser LANGUAGES CXX)

find_package(GTest REQUIRED)

message(STATUS "GTest_FOUND : ${GTest_FOUND}")



file(DOWNLOAD https://publicsuffix.org/list/public_suffix_list.dat public_suffix_list.dat)

file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

#add_library(public_suffix_list OBJECT public_suffix_list.dat)
add_library(urlparser SHARED ${SOURCES}
#        $<TARGET_OBJECTS:public_suffix_list>
)
add_library(url::base ALIAS urlparser)

target_include_directories(urlparser
        PUBLIC
            ${PROJECT_SOURCE_DIR}/include
        PRIVATE
            ${PROJECT_SOURCE_DIR}/src)


if(DEFINED SKBUILD)
    # find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
    # find_package(pybind11 CONFIG REQUIRED)
    # python_add_library(_core MODULE src/${SKBUILD_PROJECT_NAME}/main.cpp WITH_SOABI)
    # target_link_libraries(_core PRIVATE pybind11::headers)
    # target_compile_definitions(_core PRIVATE VERSION_INFO=${SKBUILD_PROJECT_VERSION})
    # target_link_libraries(_core PRIVATE url::base)
    # install(TARGETS _core urlparser DESTINATION ${SKBUILD_PROJECT_NAME})


    set(PYBIND11_NEWPYTHON ON)
    find_package(pybind11 CONFIG REQUIRED)
    pybind11_add_module(_core src/${SKBUILD_PROJECT_NAME}/main.cpp)
    target_sources(_core PRIVATE ${SOURCES})
    target_include_directories(_core PRIVATE include src)
    target_compile_definitions(_core PRIVATE VERSION_INFO=${SKBUILD_PROJECT_VERSION})
    install(TARGETS _core DESTINATION ${SKBUILD_PROJECT_NAME})
    
    RETURN()
endif()

add_executable(example examples/main.cpp)
# target_sources(example PRIVATE ${SOURCES})
# target_include_directories(example
#         PUBLIC
#             ${PROJECT_SOURCE_DIR}/include
#         PRIVATE
#             ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(example PRIVATE url::base)


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




