cmake_minimum_required(VERSION 3.19)

# ==============================================================================
# liburlparser Benchmarks — fully standalone CMake project.
#
# This has NO relationship to the repository root — not even for liburlparser
# itself. Every language folder fetches liburlparser the exact same way it
# fetches any competitor: from https://github.com/mohammadraziei/liburlparser.git
# (C++, via FetchContent) or `pip install git+https://github.com/mohammadraziei/
# liburlparser.git` (Python). liburlparser is never a special case here — it's
# benchmarked exactly like ada or tldextract: one more entry in the same
# results table. (Mirrors the same structure as ctoon's own benchmarks/.)
#
# Build it on its own:
#
#   cd benchmarks
#   cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-bench --target urlparser_benchmarks
#
# Layout: one folder per language (cpp, python), each with its own
# CMakeLists.txt that fetches its own dependencies and defines an
# urlparser_benchmarks_<lang> target. A language is skipped with a warning
# only when its toolchain isn't found.
# ==============================================================================

project(UrlParserBenchmarks LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(WARNING "Benchmarks are being built in Debug mode — numbers will "
                     "not reflect real performance. Pass -DCMAKE_BUILD_TYPE=Release.")
endif()

set(URLPARSER_BENCH_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(URLPARSER_BENCH_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(URLPARSER_GITHUB_URL "https://github.com/mohammadraziei/liburlparser.git")

# Every language benchmark writes its own JSON results file here — kept
# separate per language rather than merged, since the set of libraries each
# language tests differs.
set(URLPARSER_BENCH_RESULTS_DIR ${URLPARSER_BENCH_BINARY_DIR}/results)
file(MAKE_DIRECTORY ${URLPARSER_BENCH_RESULTS_DIR})

# ── Toolchain detection ─────────────────────────────────────────────────
# A language is skipped (with a warning) only when its toolchain itself
# isn't found. Everything else (venvs, pip installs, FetchContent) happens
# inside that language's own CMakeLists.txt once we know the toolchain
# exists. Needed here too, now, since corpus generation below runs Python.
find_package(Python3 COMPONENTS Interpreter)

# ── Corpus: fetch a real-world domain list, derive a URL list from it ─────
# 10,000 real, currently-registered domains (Majestic Million-derived),
# covering a wide range of TLDs/ccTLDs and subdomain shapes — not synthetic
# data generated just for this benchmark.
#
# Shared, single location for the downloaded/derived corpus - NOT per
# build directory (${URLPARSER_BENCH_BINARY_DIR} changes for every fresh
# `cmake -B <dir>`, which used to mean a fresh multi-hundred-KB download
# every time). Every build reuses this one copy; .gitignore'd, never
# committed. Standalone (non-CMake) tools under benchmarks/ read from here
# too - this is the single source of truth for the corpus, nothing
# downloads or derives it a second way anywhere else.
#
# The actual download + derivation logic lives in corpus/generate_corpus.py,
# not here: string/URL construction is a poor fit for CMake's DSL, and the
# derived urls.txt is round-tripped through ada_url's WHATWG-spec parser
# there (the same URL library this project's own benchmarks compare
# against) rather than trusted as hand-built strings.
set(URLPARSER_BENCH_CORPUS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/corpus)
set(URLPARSER_BENCH_DOMAINS_FILE ${URLPARSER_BENCH_CORPUS_DIR}/domains.txt)
set(URLPARSER_BENCH_URLS_FILE ${URLPARSER_BENCH_CORPUS_DIR}/urls.txt)

if(Python3_Interpreter_FOUND AND (NOT EXISTS ${URLPARSER_BENCH_DOMAINS_FILE} OR NOT EXISTS ${URLPARSER_BENCH_URLS_FILE}))
    # ada-url is a ~2MB prebuilt wheel with zero dependencies of its own -
    # installed straight into the system/found Python3, no dedicated venv
    # for it. (The separate, heavier venv in python/CMakeLists.txt -
    # nanobind, scikit-build-core, tldextract, ... - is for actually
    # *running* the Python benchmark later; unrelated to this one-off
    # corpus-generation step and not worth sharing across the two.)
    execute_process(COMMAND ${Python3_EXECUTABLE} -c "import ada_url" RESULT_VARIABLE _ada_url_present)
    if(NOT _ada_url_present EQUAL 0)
        execute_process(COMMAND ${Python3_EXECUTABLE} -m pip install -q --disable-pip-version-check --user ada-url
                         RESULT_VARIABLE _pip_rc)
        if(NOT _pip_rc EQUAL 0)
            # PEP 668 ("externally-managed-environment") refuses a plain
            # --user install on modern Debian/Ubuntu system Pythons -
            # --break-system-packages is the documented escape hatch for
            # exactly this: one small, dependency-free utility package,
            # not a project dependency going into the system environment.
            execute_process(
                COMMAND ${Python3_EXECUTABLE} -m pip install -q --disable-pip-version-check --user
                        --break-system-packages ada-url
                RESULT_VARIABLE _pip_rc)
        endif()
        if(NOT _pip_rc EQUAL 0)
            message(FATAL_ERROR "Failed to install ada-url (needed by benchmarks/corpus/generate_corpus.py)")
        endif()
    endif()

    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${URLPARSER_BENCH_CORPUS_DIR}/generate_corpus.py
        RESULT_VARIABLE _corpus_rc
    )
    if(NOT _corpus_rc EQUAL 0)
        message(FATAL_ERROR "Failed to generate benchmark corpus")
    endif()
elseif(NOT Python3_Interpreter_FOUND AND (NOT EXISTS ${URLPARSER_BENCH_DOMAINS_FILE} OR NOT EXISTS ${URLPARSER_BENCH_URLS_FILE}))
    message(FATAL_ERROR "Python3 is required to generate the benchmark corpus (benchmarks/corpus/generate_corpus.py)")
endif()

file(STRINGS ${URLPARSER_BENCH_DOMAINS_FILE} URLPARSER_BENCH_DOMAINS)
list(LENGTH URLPARSER_BENCH_DOMAINS URLPARSER_BENCH_DOMAIN_COUNT)
message(STATUS "liburlparser benchmarks: domain corpus has ${URLPARSER_BENCH_DOMAIN_COUNT} domains")

add_custom_target(urlparser_benchmarks
    COMMENT "Running all available liburlparser language benchmarks"
)


# C++ always runs: this project itself requires a CXX compiler to configure
# at all, so there's nothing to conditionally detect.
add_subdirectory(cpp)

if(Python3_Interpreter_FOUND)
    add_subdirectory(python)
else()
    message(WARNING "Python3 not found — Python benchmark skipped.")
endif()
