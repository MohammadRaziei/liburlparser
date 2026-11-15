# Performance regression guard - a tripwire, not a benchmark (see the file's own header
# comment for what it does and does not catch). Always built at -O3 regardless of the
# parent build's CMAKE_BUILD_TYPE or coverage flags: test_liburlparser_cpp sometimes builds
# at -O0 for coverage instrumentation, where a timing assertion would be meaningless.
# Links directly against this source tree's src/urlparser.cpp - unlike benchmarks/cpp/,
# which FetchContent's a fresh copy from GitHub to compare against competitors, this
# needs to test the code actually being changed.
add_executable(perf_regression_guard perf_regression_guard.cpp ${PROJECT_SOURCE_DIR}/src/urlparser.cpp)
target_include_directories(perf_regression_guard PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${GENERATED_HEADERS_DIR}
)
target_compile_options(perf_regression_guard PRIVATE -O3)
target_compile_definitions(perf_regression_guard PRIVATE URLPARSER_STATIC)
add_test(NAME perf_regression_guard COMMAND $<TARGET_FILE:perf_regression_guard>)
