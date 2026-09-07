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

# ── Corpus: fetch a real-world domain list to benchmark against ───────────
# 10,000 real, currently-registered domains (Majestic Million-derived),
# covering a wide range of TLDs/ccTLDs and subdomain shapes — not synthetic
# data generated just for this benchmark.
set(URLPARSER_BENCH_DOMAINS_FILE ${URLPARSER_BENCH_BINARY_DIR}/domains.txt)
if(NOT EXISTS ${URLPARSER_BENCH_DOMAINS_FILE})
    file(DOWNLOAD
        "https://raw.githubusercontent.com/felmoltor/robotstxt/master/top.10000.majestic.domains.txt"
        ${URLPARSER_BENCH_DOMAINS_FILE}
        STATUS _download_status
        TLS_VERIFY ON
    )
    list(GET _download_status 0 _download_code)
    if(NOT _download_code EQUAL 0)
        message(FATAL_ERROR "Failed to download domain corpus: ${_download_status}")
    endif()
endif()

file(STRINGS ${URLPARSER_BENCH_DOMAINS_FILE} URLPARSER_BENCH_DOMAINS)
list(LENGTH URLPARSER_BENCH_DOMAINS URLPARSER_BENCH_DOMAIN_COUNT)
message(STATUS "liburlparser benchmarks: domain corpus has ${URLPARSER_BENCH_DOMAIN_COUNT} domains")

# Derive a full-URL corpus from the same real domains — varied paths,
# query strings, and a "www." subdomain on every third entry, so the URL
# parse benchmark isn't just "scheme + bare host" for every row.
set(URLPARSER_BENCH_URLS_FILE ${URLPARSER_BENCH_BINARY_DIR}/urls.txt)
if(NOT EXISTS ${URLPARSER_BENCH_URLS_FILE})
    set(_urls_content "")
    set(_i 0)
    foreach(_domain ${URLPARSER_BENCH_DOMAINS})
        math(EXPR _mod3 "${_i} % 3")
        math(EXPR _mod4 "${_i} % 4")
        if(_mod3 EQUAL 0)
            set(_host "www.${_domain}")
        else()
            set(_host "${_domain}")
        endif()
        if(_mod4 EQUAL 0)
            set(_url "https://${_host}/")
        elseif(_mod4 EQUAL 1)
            set(_url "https://${_host}/index.html?ref=bench&id=${_i}")
        elseif(_mod4 EQUAL 2)
            set(_url "http://${_host}/articles/${_i}/view")
        else()
            set(_url "https://${_host}/search?q=test+${_i}#results")
        endif()
        string(APPEND _urls_content "${_url}\n")
        math(EXPR _i "${_i} + 1")
    endforeach()
    file(WRITE ${URLPARSER_BENCH_URLS_FILE} "${_urls_content}")
endif()

# ── Toolchain detection ─────────────────────────────────────────────────
# A language is skipped (with a warning) only when its toolchain itself
# isn't found. Everything else (venvs, pip installs, FetchContent) happens
# inside that language's own CMakeLists.txt once we know the toolchain
# exists.
find_package(Python3 COMPONENTS Interpreter)

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
