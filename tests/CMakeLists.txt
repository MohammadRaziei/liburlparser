set(URLPARSER_TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(URLPARSER_TEST_BINARY_DIR ${CMAKE_BINARY_DIR}/tests)
set(URLPARSER_COVERAGE_BINARY_DIR ${CMAKE_BINARY_DIR}/coverage)

set(URLPARSER_COVERAGE_TOTAL_BINARY_DIR ${URLPARSER_COVERAGE_BINARY_DIR}/total CACHE PATH "Total coverage output directory")
set(URLPARSER_COVERAGE_CPP_BINARY_DIR ${URLPARSER_COVERAGE_BINARY_DIR}/cpp CACHE PATH "C++ coverage output directory")
set(URLPARSER_COVERAGE_PYTHON_BINARY_DIR ${URLPARSER_COVERAGE_BINARY_DIR}/python CACHE PATH "Python coverage output directory")

file(REMOVE_RECURSE ${URLPARSER_COVERAGE_BINARY_DIR})
file(MAKE_DIRECTORY ${URLPARSER_COVERAGE_BINARY_DIR})

add_custom_target(urlparser_test
    COMMAND ${CMAKE_COMMAND} -E echo "-- Running All tests..."
)

add_custom_target(urlparser_coverage
    COMMAND ${CMAKE_COMMAND} -E echo "-- Running coverage for all languages ..."
)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_program(LCOV_EXECUTABLE lcov)
find_program(GENHTML_EXECUTABLE genhtml)

add_subdirectory(${URLPARSER_TEST_SOURCE_DIR}/cpp)
add_subdirectory(${URLPARSER_TEST_SOURCE_DIR}/python)

if(TARGET urlparser_coverage_cpp AND TARGET urlparser_coverage_python)
    if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
        add_custom_target(urlparser_coverage_total
            # Clean previous coverage data
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${URLPARSER_COVERAGE_TOTAL_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${URLPARSER_COVERAGE_TOTAL_BINARY_DIR}/html

            # Merge all coverage info files
            COMMAND ${CMAKE_COMMAND} -E echo "-- Merging C++ and Python coverage..."
            COMMAND ${LCOV_EXECUTABLE}
                    --add-tracefile ${URLPARSER_COVERAGE_CPP_BINARY_DIR}/coverage.lcov
                    --add-tracefile ${URLPARSER_COVERAGE_PYTHON_BINARY_DIR}/coverage.lcov
                    --output-file ${URLPARSER_COVERAGE_TOTAL_BINARY_DIR}/coverage.lcov
            COMMAND ${CMAKE_COMMAND} -E echo "-- Total coverage info created at: ${URLPARSER_COVERAGE_TOTAL_BINARY_DIR}/coverage.lcov"

            # Generate HTML report
            COMMAND ${CMAKE_COMMAND} -E echo "-- Generating HTML report..."
            COMMAND ${GENHTML_EXECUTABLE} --quiet
                    ${URLPARSER_COVERAGE_TOTAL_BINARY_DIR}/coverage.lcov
                    --output-directory ${URLPARSER_COVERAGE_TOTAL_BINARY_DIR}/html
                    --prefix ${PROJECT_SOURCE_DIR}
                    --title "liburlparser Total Coverage"
                    --css-file ${URLPARSER_TEST_SOURCE_DIR}/genhtml.css
                    --dark-mode
            COMMAND ${CMAKE_COMMAND}
                -DCOV_FILE=${URLPARSER_COVERAGE_TOTAL_BINARY_DIR}/coverage.lcov
                -DSOURCE_PATH=${PROJECT_SOURCE_DIR}/
                -P ${PROJECT_SOURCE_DIR}/cmake/FixLCovPaths.cmake
            COMMAND ${CMAKE_COMMAND} -E echo "-- HTML entry point created here: ${URLPARSER_COVERAGE_TOTAL_BINARY_DIR}/html/index.html"

            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            USES_TERMINAL
        )

        add_dependencies(urlparser_coverage_total urlparser_coverage_cpp urlparser_coverage_python)
        add_dependencies(urlparser_coverage urlparser_coverage_total)
    endif()
endif()
