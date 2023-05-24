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
