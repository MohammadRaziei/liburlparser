cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)

file(DOWNLOAD https://publicsuffix.org/list/public_suffix_list.dat public_suffix_list.dat)

project(DomainExtractor LANGUAGES CXX)

find_package(GTest REQUIRED)

message(STATUS "GTest_FOUND : ${GTest_FOUND}")

file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_library(urlparser SHARED ${SOURCES})
add_library(url::base ALIAS urlparser)

target_include_directories(urlparser
        PUBLIC
            ${PROJECT_SOURCE_DIR}/include
        PRIVATE
            ${PROJECT_SOURCE_DIR}/src)

add_executable(DomainExtractor main.cpp)
target_link_libraries(DomainExtractor PRIVATE url::base)

enable_testing()
add_executable(DomainExtractorTests tests/test.cpp)
target_link_libraries(DomainExtractorTests PRIVATE url::base gtest pthread)

add_test(NAME unitTests COMMAND DomainExtractorTests)


install(TARGETS urlparser
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include)

# Find Doxygen package
find_package(Doxygen)
message(STATUS "Doxygen_FOUND : ${Doxygen_FOUND}")


# if(DOXYGEN_FOUND)
#     # Set input and output directories
#     set(DOXYGEN_INPUT_DIR "${CMAKE_SOURCE_DIR}")
#     set(DOXYGEN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/docs")

#     # Configure Doxyfile from template
#     configure_file(Doxyfile.in ${CMAKE_BINARY_DIR}/Doxyfile @ONLY)

#     # Add Doxygen target
#     add_custom_target(docs
#         COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
#         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#         COMMENT "Generating Doxygen documentation"
#         VERBATIM
#     )
# endif()

# Find Doxygen package
find_package(Doxygen REQUIRED)

# Configure Doxyfile.in template
configure_file(Doxyfile.in ${CMAKE_BINARY_DIR}/Doxyfile @ONLY)

# Add target to generate Doxyfile
add_custom_target(generate_doxyfile
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/Doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    COMMENT "Generating Doxyfile"
)

# Add target to generate documentation
add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating Doxygen documentation"
    VERBATIM
)

