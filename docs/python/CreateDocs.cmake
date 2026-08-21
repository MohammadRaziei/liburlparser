# CreateDocs.cmake — Python documentation (Sphinx)
# Called via: cmake -P CreateDocs.cmake
# Required CMake variables (passed via -D):
#   PYTHON_EXECUTABLE            — Python interpreter
#   SOURCE_DIR                   — Sphinx source dir (contains conf.py)
#   OUTPUT_DIR                   — Output directory for generated HTML
#   URLPARSER_PYTHON_INSTALL_DIR — Path where liburlparser was installed
#                                  (BUILD_PYTHON target, ${CMAKE_BINARY_DIR})

cmake_minimum_required(VERSION 3.19)

# ── Validate ──────────────────────────────────────────────────────────────────
foreach(_var PYTHON_EXECUTABLE SOURCE_DIR OUTPUT_DIR URLPARSER_PYTHON_INSTALL_DIR)
    if(NOT DEFINED ${_var})
        message(FATAL_ERROR "CreateDocs.cmake (python): ${_var} must be defined")
    endif()
endforeach()

if(NOT EXISTS "${URLPARSER_PYTHON_INSTALL_DIR}")
    message(FATAL_ERROR
        "URLPARSER_PYTHON_INSTALL_DIR does not exist: ${URLPARSER_PYTHON_INSTALL_DIR}\n"
        "Configure/build with -DBUILD_PYTHON=on first."
    )
endif()

# ── Check Sphinx ──────────────────────────────────────────────────────────────
execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" -c "import sphinx"
    RESULT_VARIABLE _SPHINX_CHECK
    OUTPUT_QUIET ERROR_QUIET
)
if(NOT _SPHINX_CHECK EQUAL 0)
    message(FATAL_ERROR "Sphinx is not installed. Run: pip install sphinx myst-parser furo")
endif()

# ── Prepare output dirs ───────────────────────────────────────────────────────
set(PY_HTML      "${OUTPUT_DIR}/html")
set(PY_DOCTREES  "${OUTPUT_DIR}/doctrees")

file(MAKE_DIRECTORY "${PY_HTML}")
file(MAKE_DIRECTORY "${PY_DOCTREES}")

# ── Run Sphinx ────────────────────────────────────────────────────────────────
# Working dir = the install dir so `import liburlparser` (in conf.py) picks up
# the freshly-built extension, not some other copy on the path.
execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -m sphinx
            -b html
            -d "${PY_DOCTREES}"
            "${SOURCE_DIR}"
            "${PY_HTML}"
    WORKING_DIRECTORY ${URLPARSER_PYTHON_INSTALL_DIR}
    RESULT_VARIABLE SPHINX_RESULT
)

if(NOT SPHINX_RESULT EQUAL 0)
    message(FATAL_ERROR "Sphinx failed with exit code ${SPHINX_RESULT}")
endif()

message(STATUS "Python docs generated at: ${PY_HTML}/index.html")
