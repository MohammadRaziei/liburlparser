# --- Testing and Coverage Section ---
# Uses the package that was already assembled by the main CMakeLists.txt
# under ${CMAKE_BINARY_DIR}/${PROJECT_NAME} when BUILD_PYTHON=on.
set(URLPARSER_PYTHON_INSTALL_DIR ${CMAKE_BINARY_DIR})

if(BUILD_PYTHON)
    # Check for pytest
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -m pytest --version
        OUTPUT_QUIET
        ERROR_QUIET
        RESULT_VARIABLE PYTEST_CHECK_RESULT
    )

    if(NOT PYTEST_CHECK_RESULT EQUAL 0)
        message(WARNING "pytest is not installed. Python tests will be skipped.")
        message(WARNING "Please install it using: pip install pytest pytest-xdist")
    else()
        set(URLPARSER_PYTEST_ARGS -v -n auto ${CMAKE_CURRENT_SOURCE_DIR})

        # memray has no Windows support, so it's never even checked there.
        # Otherwise: if pytest-memray is installed, fold its leak checking
        # (@pytest.mark.limit_leaks in test_memory.py) into the normal test
        # run via --memray; if not installed, just run pytest normally.
        if(NOT WIN32)
            execute_process(
                COMMAND ${Python3_EXECUTABLE} -c "import pytest_memray"
                OUTPUT_QUIET
                ERROR_QUIET
                RESULT_VARIABLE MEMRAY_CHECK_RESULT
            )
            if(MEMRAY_CHECK_RESULT EQUAL 0)
                list(APPEND URLPARSER_PYTEST_ARGS --memray)
            else()
                message(STATUS "pytest-memray not installed - running python tests without leak checking.")
                message(STATUS "Please install it using: pip install -r requirements-dev.txt")
            endif()
        endif()

        add_test(NAME test_liburlparser_python
                 COMMAND ${Python3_EXECUTABLE} -m pytest ${URLPARSER_PYTEST_ARGS}
                 WORKING_DIRECTORY ${URLPARSER_PYTHON_INSTALL_DIR}
        )

        add_custom_target(urlparser_test_python
            COMMAND ${CMAKE_COMMAND} -E echo "-- Running python tests..."
            COMMAND ${Python3_EXECUTABLE} -m pytest ${URLPARSER_PYTEST_ARGS}
            WORKING_DIRECTORY ${URLPARSER_PYTHON_INSTALL_DIR}
            USES_TERMINAL
        )

        add_dependencies(urlparser_test urlparser_test_python)
        if(DEFINED NB_MODULE)
            add_dependencies(urlparser_test_python ${NB_MODULE})
        endif()
    endif()

    # Check for coverage.py
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -m coverage --version
        OUTPUT_QUIET
        ERROR_QUIET
        RESULT_VARIABLE COVERAGE_CHECK_RESULT
    )

    if(COVERAGE_CHECK_RESULT EQUAL 0)
        add_custom_target(urlparser_coverage_python
            # Clean previous coverage data (only html, keep index.html wrapper)
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${URLPARSER_COVERAGE_PYTHON_BINARY_DIR}/html
            COMMAND ${CMAKE_COMMAND} -E make_directory ${URLPARSER_COVERAGE_PYTHON_BINARY_DIR}/html

            # Run tests with coverage enabled, from the install dir so Python
            # can find the assembled package.
            COMMAND ${CMAKE_COMMAND} -E echo "-- Capturing coverage..."
            COMMAND ${Python3_EXECUTABLE} -m coverage run
                    --source=${URLPARSER_PYTHON_INSTALL_DIR}/${PROJECT_NAME}
                    --omit="*/tests/*"
                    -m pytest ${CMAKE_CURRENT_SOURCE_DIR}

            # Generate lcov file
            COMMAND ${CMAKE_COMMAND} -E echo "-- Generating coverage.lcov file..."
            COMMAND ${Python3_EXECUTABLE} -m coverage lcov -o ${URLPARSER_COVERAGE_PYTHON_BINARY_DIR}/coverage.lcov

            # coverage.py records paths relative to the build dir it ran in
            # (e.g. "liburlparser/core.py"), but the real source lives at
            # "src/liburlparser/core.py" - prepend that prefix so paths
            # resolve correctly once merged with the C++ coverage (which is
            # already normalized to be PROJECT_SOURCE_DIR-relative).
            COMMAND ${CMAKE_COMMAND}
                    -DCOV_FILE=${URLPARSER_COVERAGE_PYTHON_BINARY_DIR}/coverage.lcov
                    -DSOURCE_PATH=
                    -DPREFIX_PATH=src/
                    -P ${PROJECT_SOURCE_DIR}/cmake/FixLCovPaths.cmake

            # Generate HTML report
            COMMAND ${CMAKE_COMMAND} -E echo "-- Generating HTML report..."
            COMMAND ${Python3_EXECUTABLE} -m coverage html -d ${URLPARSER_COVERAGE_PYTHON_BINARY_DIR}/html

            # Summary
            COMMAND ${CMAKE_COMMAND} -E echo "-- HTML entry point created here: ${URLPARSER_COVERAGE_PYTHON_BINARY_DIR}/html/index.html"
            COMMAND ${CMAKE_COMMAND} -E echo "-- Coverage summary:"
            COMMAND ${Python3_EXECUTABLE} -m coverage report

            WORKING_DIRECTORY ${URLPARSER_PYTHON_INSTALL_DIR}
            USES_TERMINAL
        )

        add_dependencies(urlparser_coverage urlparser_coverage_python)
        if(DEFINED NB_MODULE)
            add_dependencies(urlparser_coverage_python ${NB_MODULE})
        endif()
    else()
        message(WARNING "coverage module is not installed. Python coverage target will be skipped.")
        message(WARNING "Please install it using: pip install coverage")
    endif()
else()
    message(WARNING "BUILD_PYTHON is off - python tests/coverage will be skipped. Configure with -DBUILD_PYTHON=on")
endif()
