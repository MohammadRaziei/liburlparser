include(FetchContent)

# liburlparser itself — fetched exactly like any competitor, from its own
# GitHub repo, never a special case.
FetchContent_Declare(
    liburlparser
    GIT_REPOSITORY ${URLPARSER_GITHUB_URL}
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

# ada (github.com/ada-url/ada) — WHATWG-compliant URL parser used in
# Node.js, Cloudflare Workers, ClickHouse, Redpanda, and others. Disable
# its own tests/tools/benchmarks so it doesn't pull in CPM/googletest.
set(ADA_TESTING OFF CACHE BOOL "" FORCE)
set(ADA_TOOLS OFF CACHE BOOL "" FORCE)
set(ADA_BENCHMARKS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    ada
    GIT_REPOSITORY https://github.com/ada-url/ada.git
    GIT_TAG        v3.4.4
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(liburlparser ada)

add_executable(bench_cpp bench.cpp)
target_link_libraries(bench_cpp PRIVATE url::base ada::ada)
target_compile_definitions(bench_cpp PRIVATE
    URLPARSER_BENCH_DOMAINS_FILE="${URLPARSER_BENCH_DOMAINS_FILE}"
    URLPARSER_BENCH_URLS_FILE="${URLPARSER_BENCH_URLS_FILE}"
    URLPARSER_BENCH_RESULTS_JSON="${URLPARSER_BENCH_RESULTS_DIR}/cpp.json")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(bench_cpp PRIVATE -O3)
endif()

add_custom_target(urlparser_benchmarks_cpp
    COMMAND ${CMAKE_COMMAND} -E echo "-- Running C++ benchmark..."
    COMMAND $<TARGET_FILE:bench_cpp>
    DEPENDS bench_cpp
    USES_TERMINAL
)
add_dependencies(urlparser_benchmarks urlparser_benchmarks_cpp)
