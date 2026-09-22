# Find all test source files matching test_*.cpp pattern
file(GLOB URLPARSER_TEST_SOURCES_CPP CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp
)

add_executable(test_liburlparser_cpp
    ${URLPARSER_TEST_SOURCES_CPP}
    ${PROJECT_SOURCE_DIR}/src/urlparser.cpp
    ${PROJECT_SOURCE_DIR}/src/idna.cpp
    ${PROJECT_SOURCE_DIR}/src/percent_codec.cpp
    ${URLPARSER_TEST_SOURCE_DIR}/utest/utest_main.cxx
)

target_include_directories(test_liburlparser_cpp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${URLPARSER_TEST_SOURCE_DIR}/utest
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${GENERATED_HEADERS_DIR}
)

target_link_libraries(test_liburlparser_cpp PRIVATE Threads::Threads)

add_test(NAME test_liburlparser_cpp COMMAND $<TARGET_FILE:test_liburlparser_cpp>)

# Performance regression guard - a tripwire, not a benchmark (see the file's
# own header comment for what it does and doesn't catch). Always built at
# -O3 regardless of the parent build's CMAKE_BUILD_TYPE or coverage flags -
# unlike test_liburlparser_cpp above, which sometimes builds at -O0 for
# coverage instrumentation, where a timing assertion would be meaningless.
# Links directly against this source tree's src/urlparser.cpp - unlike
# benchmarks/cpp/, which FetchContent's a fresh copy from GitHub to compare
# against competitors, this needs to test the code actually being changed.
add_executable(perf_regression_guard perf_regression_guard.cpp ${PROJECT_SOURCE_DIR}/src/urlparser.cpp)
target_include_directories(perf_regression_guard PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${GENERATED_HEADERS_DIR}
)
target_compile_options(perf_regression_guard PRIVATE -O3)
add_test(NAME perf_regression_guard COMMAND $<TARGET_FILE:perf_regression_guard>)

# Randomized property test for url::abspath() against an independent
# reference implementation (see the file's header comment) - cheap enough
# (~0.2s for 200k iterations) to run on every ctest invocation with a fixed
# seed for reproducibility, not just as an opt-in fuzzing target.
add_executable(fuzz_abspath fuzz_abspath.cpp ${PROJECT_SOURCE_DIR}/src/urlparser.cpp)
target_include_directories(fuzz_abspath PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${GENERATED_HEADERS_DIR}
)
add_test(NAME fuzz_abspath COMMAND $<TARGET_FILE:fuzz_abspath> 42 300000)

add_custom_target(urlparser_test_cpp
    COMMAND ${CMAKE_COMMAND} -E echo "-- Running C++ tests..."
    COMMAND $<TARGET_FILE:test_liburlparser_cpp>
    WORKING_DIRECTORY ${URLPARSER_TEST_BINARY_DIR}
)
add_dependencies(urlparser_test_cpp test_liburlparser_cpp)
add_dependencies(urlparser_test urlparser_test_cpp)


if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
    # Enable coverage flags for the test executable
    target_compile_options(test_liburlparser_cpp PRIVATE -O0 -g --coverage)
    target_link_options(test_liburlparser_cpp PRIVATE --coverage)

    add_custom_target(urlparser_coverage_cpp
        # Clean previous coverage data (only html, keep index.html wrapper)
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${URLPARSER_COVERAGE_CPP_BINARY_DIR}/html
        COMMAND ${CMAKE_COMMAND} -E make_directory ${URLPARSER_COVERAGE_CPP_BINARY_DIR}/html

        # Capture coverage data
        COMMAND ${CMAKE_COMMAND} -E echo "-- Capturing coverage..."
        COMMAND ${LCOV_EXECUTABLE}
                --capture
                --base-directory ${PROJECT_SOURCE_DIR}
                --directory ${CMAKE_BINARY_DIR}
                --exclude "*/tests/*"
                --no-external
                --rc function_coverage=1
                --output-file ${URLPARSER_COVERAGE_CPP_BINARY_DIR}/coverage_temp.lcov
        # Extract only liburlparser source files (library headers/sources)
        COMMAND ${CMAKE_COMMAND} -E echo "Filtering liburlparser source files..."
        COMMAND ${LCOV_EXECUTABLE}
                --extract
                ${URLPARSER_COVERAGE_CPP_BINARY_DIR}/coverage_temp.lcov
                "*/include/*" "*/src/*"
                --output-file ${URLPARSER_COVERAGE_CPP_BINARY_DIR}/coverage.lcov
        COMMAND ${CMAKE_COMMAND} -E remove ${URLPARSER_COVERAGE_CPP_BINARY_DIR}/coverage_temp.lcov

        # Generate HTML report
        COMMAND ${CMAKE_COMMAND} -E echo "-- Generating HTML report..."
        COMMAND ${GENHTML_EXECUTABLE} --quiet
                ${URLPARSER_COVERAGE_CPP_BINARY_DIR}/coverage.lcov
                --output-directory ${URLPARSER_COVERAGE_CPP_BINARY_DIR}/html
                --prefix ${PROJECT_SOURCE_DIR}
                --title "liburlparser C++ Coverage"
                --css-file ${URLPARSER_TEST_SOURCE_DIR}/genhtml.css
                --dark-mode
        COMMAND ${CMAKE_COMMAND} -E echo "-- HTML entry point created here: ${URLPARSER_COVERAGE_CPP_BINARY_DIR}/html/index.html"

        # Fix paths to be relative (for SonarQube, etc.)
        COMMAND ${CMAKE_COMMAND} -E echo "-- Fixing paths to relative..."
        COMMAND ${CMAKE_COMMAND}
                -DCOV_FILE=${URLPARSER_COVERAGE_CPP_BINARY_DIR}/coverage.lcov
                -DSOURCE_PATH=${PROJECT_SOURCE_DIR}/
                -P ${PROJECT_SOURCE_DIR}/cmake/FixLCovPaths.cmake
        # Summary
        COMMAND ${CMAKE_COMMAND} -E echo "-- Coverage summary:"
        COMMAND ${LCOV_EXECUTABLE}
                --list
                ${URLPARSER_COVERAGE_CPP_BINARY_DIR}/coverage.lcov

        # Wrap the raw genhtml report in a themed iframe page at cpp/index.html
        COMMAND ${CMAKE_COMMAND}
                -DIFRAME_SRC=html/index.html
                -DSECTION_TITLE=C++%20Coverage
                -DLOGO_SVG=${PROJECT_SOURCE_DIR}/docs/images/logo/liburlparser-logo-1.svg
                -DPROJECT_NAME=liburlparser
                -DOUTPUT_DIR=${URLPARSER_COVERAGE_CPP_BINARY_DIR}
                -P ${PROJECT_SOURCE_DIR}/cmake/GenerateIframe.cmake

        WORKING_DIRECTORY ${URLPARSER_TEST_BINARY_DIR}
        USES_TERMINAL
    )

    add_dependencies(urlparser_coverage_cpp urlparser_test_cpp)
    add_dependencies(urlparser_coverage urlparser_coverage_cpp)
else()
    message(WARNING "LCOV or GENHTML executables not found.")
    message(WARNING "C++ coverage target will be skipped.")
    message(WARNING "Please install them to enable coverage reports (e.g., 'sudo apt install lcov' on Linux).")
endif()
