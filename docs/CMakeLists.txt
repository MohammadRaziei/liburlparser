# liburlparser Documentation Build
# Builds C++ docs (Doxygen) via CreateDocs.cmake, mirroring ctoon's docs/CMakeLists.txt.
#
# NOTE: liburlparser's Python docs (docs/python) are an mkdocs site, not Sphinx,
# and are built/deployed separately (mkdocs build / ReadTheDocs) - they aren't
# wired into this CMake target.

cmake_minimum_required(VERSION 3.19)

set(URLPARSER_DOCS_OUT ${CMAKE_CURRENT_BINARY_DIR}/out)

set(URLPARSER_DOCS_CPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cpp)
set(URLPARSER_DOCS_CPP_OUT ${URLPARSER_DOCS_OUT}/cpp)

# ── C++ Documentation (Doxygen) ───────────────────────────────

find_package(Doxygen)

if(DOXYGEN_FOUND)
    set(URLPARSER_DOCS_CPP_HTML ${URLPARSER_DOCS_CPP_OUT}/html/index.html)

    add_custom_command(
        OUTPUT ${URLPARSER_DOCS_CPP_HTML}
        COMMAND ${CMAKE_COMMAND}
            -DPROJECT_NAME=${PROJECT_NAME}
            -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
            -DPROJECT_VERSION=${PROJECT_VERSION}
            -DOUTPUT_DIR=${URLPARSER_DOCS_CPP_OUT}
            -DDOXYGEN_EXECUTABLE=${DOXYGEN_EXECUTABLE}
            -DFOOTER_IN=${URLPARSER_DOCS_CPP_DIR}/footer.html.in
            -DDOXYFILE_IN=${URLPARSER_DOCS_CPP_DIR}/Doxyfile.in
            -DLOGO_SRC=${PROJECT_SOURCE_DIR}/docs/images/logo/liburlparser-logo-2.svg
            -P ${URLPARSER_DOCS_CPP_DIR}/CreateDocs.cmake
        DEPENDS
            ${URLPARSER_DOCS_CPP_DIR}/CreateDocs.cmake
            ${URLPARSER_DOCS_CPP_DIR}/footer.html.in
            ${URLPARSER_DOCS_CPP_DIR}/Doxyfile.in
        COMMENT "Generating C++ API documentation with Doxygen"
        VERBATIM
    )

    add_custom_target(urlparser_docs_cpp DEPENDS ${URLPARSER_DOCS_CPP_HTML})
else()
    message(WARNING "Doxygen not found - C++ documentation will not be generated")
    add_custom_target(urlparser_docs_cpp
        COMMAND ${CMAKE_COMMAND} -E echo "Skipping C++ docs: Doxygen not found"
    )
endif()


# ── Python Documentation (Sphinx) ──────────────────────────────

set(URLPARSER_DOCS_PYTHON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/python)
set(URLPARSER_DOCS_PYTHON_OUT ${URLPARSER_DOCS_OUT}/python)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import sphinx"
    OUTPUT_QUIET
    ERROR_QUIET
    RESULT_VARIABLE SPHINX_CHECK
)

if(BUILD_PYTHON AND SPHINX_CHECK EQUAL 0)
    set(URLPARSER_DOCS_PYTHON_HTML ${URLPARSER_DOCS_PYTHON_OUT}/html/index.html)

    add_custom_command(
        OUTPUT ${URLPARSER_DOCS_PYTHON_HTML}
        COMMAND ${CMAKE_COMMAND}
            -DPYTHON_EXECUTABLE=${Python3_EXECUTABLE}
            -DSOURCE_DIR=${URLPARSER_DOCS_PYTHON_DIR}/docs
            -DOUTPUT_DIR=${URLPARSER_DOCS_PYTHON_OUT}
            -DURLPARSER_PYTHON_INSTALL_DIR=${PYTHON_PROJECT_NAME}
            -P ${URLPARSER_DOCS_PYTHON_DIR}/CreateDocs.cmake
        DEPENDS
            ${URLPARSER_DOCS_PYTHON_DIR}/CreateDocs.cmake
            ${URLPARSER_DOCS_PYTHON_DIR}/docs/conf.py
            ${URLPARSER_DOCS_PYTHON_DIR}/docs/index.md
        COMMENT "Building Python API documentation with Sphinx"
        VERBATIM
    )

    add_custom_target(urlparser_docs_python DEPENDS ${URLPARSER_DOCS_PYTHON_HTML})

    # Must have the nanobind extension(s) built/installed first so `import
    # liburlparser` works from inside conf.py.
    if(DEFINED PYTHON_BIND_MODULES)
        add_dependencies(urlparser_docs_python ${PYTHON_BIND_MODULES})
    endif()
elseif(NOT BUILD_PYTHON)
    message(WARNING "BUILD_PYTHON is off - Python documentation will not be generated. Configure with -DBUILD_PYTHON=on")
    add_custom_target(urlparser_docs_python
        COMMAND ${CMAKE_COMMAND} -E echo "Skipping Python docs: BUILD_PYTHON is off"
    )
else()
    message(WARNING "Sphinx not found (pip install sphinx myst-parser furo) - Python documentation will not be generated")
    add_custom_target(urlparser_docs_python
        COMMAND ${CMAKE_COMMAND} -E echo "Skipping Python docs: Sphinx not found"
    )
endif()


# ── Route Index Page ──────────────────────────────────────────

set(URLPARSER_INDEX_OUT ${URLPARSER_DOCS_OUT}/index.html)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/index.html.in
    ${URLPARSER_INDEX_OUT}
    @ONLY
)

add_custom_target(urlparser_docs_index DEPENDS ${URLPARSER_INDEX_OUT})


# ── Master target ─────────────────────────────────────────────

add_custom_target(urlparser_docs
    COMMAND ${CMAKE_COMMAND} -E echo "-- All documentation generated at ${URLPARSER_DOCS_OUT}"
    COMMENT "Building all documentation"
)

add_dependencies(urlparser_docs urlparser_docs_cpp urlparser_docs_python urlparser_docs_index)
