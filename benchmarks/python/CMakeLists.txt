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
#
# Split into two steps with different lifetimes:
#
# 1. Base venv + generic dependencies (tldextract, PyDomainExtractor,
#    etc. from requirements.txt) - these rarely change, so this is a
#    real add_custom_COMMAND with an OUTPUT (a stamp file), not a plain
#    add_custom_target. A target with no OUTPUT/DEPENDS of its own
#    reruns its full COMMAND list on *every* build invocation no matter
#    whether anything actually changed, which meant re-creating the
#    venv and re-running every pip install from scratch on every single
#    `cmake --build`. The stamp file makes this a normal build edge: it
#    reruns only when requirements.txt or the stamp itself is missing
#    or stale - like any other generated file.
#
# 2. liburlparser itself, straight from GitHub - the whole point of this
#    benchmark is testing whatever was most recently pushed there, so
#    unlike step 1 this one is *never* cached/skipped: it stays a plain
#    add_custom_target (always reruns) and depends on step 1 only for
#    the venv/pip to already exist, not to gate whether it reinstalls.
#
# --disable-pip-version-check (both steps) skips pip's own "check PyPI
# for a newer pip" network round-trip - unrelated to installing anything
# here, and exactly what was hanging when the network was slow/
# unreachable ("WARNING: There was an error checking the latest version
# of pip").
set(VENV_STAMP "${CMAKE_CURRENT_BINARY_DIR}/venv.stamp")
add_custom_command(
    OUTPUT "${VENV_STAMP}"
    COMMAND ${Python3_EXECUTABLE} -m venv ${VENV_DIR}
    COMMAND ${VENV_PIP} install -q --disable-pip-version-check --upgrade pip
    COMMAND ${VENV_PIP} install -q --disable-pip-version-check
            -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
    COMMAND ${CMAKE_COMMAND} -E touch "${VENV_STAMP}"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
    COMMENT "Setting up Python venv and installing benchmark dependencies"
    USES_TERMINAL
)
add_custom_target(urlparser_benchmarks_python_venv_base DEPENDS "${VENV_STAMP}")

add_custom_target(urlparser_benchmarks_python_venv
    COMMAND ${VENV_PIP} install -q --disable-pip-version-check
            --force-reinstall --no-deps "git+${URLPARSER_GITHUB_URL}"
    COMMENT "Installing the latest liburlparser from GitHub"
    USES_TERMINAL
)
add_dependencies(urlparser_benchmarks_python_venv urlparser_benchmarks_python_venv_base)

add_custom_target(urlparser_benchmarks_python
    COMMAND ${CMAKE_COMMAND} -E echo "-- Running Python benchmark..."
    COMMAND ${VENV_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/bench.py
            ${URLPARSER_BENCH_DOMAINS_FILE} ${URLPARSER_BENCH_URLS_FILE}
            ${URLPARSER_BENCH_RESULTS_DIR}/python.json
    USES_TERMINAL
)
add_dependencies(urlparser_benchmarks_python urlparser_benchmarks_python_venv)
add_dependencies(urlparser_benchmarks urlparser_benchmarks_python)


