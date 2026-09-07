cmake_path(SET VENV_DIR "${CMAKE_CURRENT_BINARY_DIR}/venv")
cmake_path(ABSOLUTE_PATH VENV_DIR NORMALIZE)

if(WIN32)
    cmake_path(APPEND VENV_DIR "Scripts" "python.exe" OUTPUT_VARIABLE VENV_PYTHON)
    cmake_path(APPEND VENV_DIR "Scripts" "pip.exe" OUTPUT_VARIABLE VENV_PIP)
else()
    cmake_path(APPEND VENV_DIR "bin" "python" OUTPUT_VARIABLE VENV_PYTHON)
    cmake_path(APPEND VENV_DIR "bin" "pip" OUTPUT_VARIABLE VENV_PIP)
endif()

# A real venv (not the system Python) — installs liburlparser and every
# competitor straight from PyPI/GitHub, exactly like every other language
# fetches its dependencies here. Nothing comes from a local checkout.
add_custom_target(urlparser_benchmarks_python_venv
    COMMAND ${Python3_EXECUTABLE} -m venv ${VENV_DIR}
    COMMAND ${VENV_PIP} install -q --upgrade pip
    COMMAND ${VENV_PIP} install -q -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
    COMMAND ${VENV_PIP} install -q --force-reinstall --no-deps
            "git+${URLPARSER_GITHUB_URL}"
    COMMENT "Setting up Python venv and installing benchmark dependencies"
    USES_TERMINAL
)

add_custom_target(urlparser_benchmarks_python
    COMMAND ${CMAKE_COMMAND} -E echo "-- Running Python benchmark..."
    COMMAND ${VENV_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/bench.py
            ${URLPARSER_BENCH_DOMAINS_FILE} ${URLPARSER_BENCH_URLS_FILE}
            ${URLPARSER_BENCH_RESULTS_DIR}/python.json
    USES_TERMINAL
)
add_dependencies(urlparser_benchmarks_python urlparser_benchmarks_python_venv)
add_dependencies(urlparser_benchmarks urlparser_benchmarks_python)
