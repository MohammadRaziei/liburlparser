cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)

#set (CMAKE_GENERATOR "Unix Makefiles" CACHE INTERNAL "" FORCE)

project(DomainExtractor)
file(DOWNLOAD https://publicsuffix.org/list/public_suffix_list.dat public_suffix_list.dat)
file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --with-jemalloc-prefix=je_")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --with-jemalloc-prefix=je_")
#add_compile_options(--with-jemalloc-prefix=je_)

add_executable(DomainExtractor main.cpp ${SOURCES} main.cpp)
target_include_directories(DomainExtractor PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src)
