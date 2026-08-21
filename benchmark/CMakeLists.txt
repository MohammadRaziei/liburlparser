# Standalone benchmark executable for liburlparser.
# Always compiled with optimizations - timing a debug build would be
# misleading regardless of what CMAKE_BUILD_TYPE the rest of the project uses.

add_executable(urlparser_benchmark main.cpp)
target_link_libraries(urlparser_benchmark PRIVATE url::base)
target_include_directories(urlparser_benchmark PRIVATE ${PROJECT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(urlparser_benchmark PRIVATE /O2)
else()
    target_compile_options(urlparser_benchmark PRIVATE -O3)
endif()

add_custom_target(urlparser_benchmark_run
    COMMAND ${CMAKE_COMMAND} -E echo "-- Running benchmark..."
    COMMAND $<TARGET_FILE:urlparser_benchmark>
    USES_TERMINAL
)
add_dependencies(urlparser_benchmark_run urlparser_benchmark)

# ── Optional profiling target (Linux, requires `perf`) ────────────────────────
find_program(PERF_EXECUTABLE perf)
if(PERF_EXECUTABLE)
    set(URLPARSER_PROFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/profile)

    add_custom_target(urlparser_profile
        COMMAND ${CMAKE_COMMAND} -E make_directory ${URLPARSER_PROFILE_OUT}
        COMMAND ${CMAKE_COMMAND} -E echo "-- Recording with perf (requires perf_event_paranoid <= 1)..."
        COMMAND ${PERF_EXECUTABLE} record -g --call-graph dwarf
                -o ${URLPARSER_PROFILE_OUT}/perf.data
                -- $<TARGET_FILE:urlparser_benchmark> 500000
        COMMAND ${CMAKE_COMMAND} -E echo "-- Writing text report to ${URLPARSER_PROFILE_OUT}/report.txt"
        COMMAND ${PERF_EXECUTABLE} report -i ${URLPARSER_PROFILE_OUT}/perf.data
                --stdio --sort=overhead,symbol > ${URLPARSER_PROFILE_OUT}/report.txt
        COMMAND ${CMAKE_COMMAND} -E echo "-- Top hotspots:"
        COMMAND ${CMAKE_COMMAND} -E env head -n 40 ${URLPARSER_PROFILE_OUT}/report.txt
        COMMAND ${CMAKE_COMMAND} -E echo "-- Full report: ${URLPARSER_PROFILE_OUT}/report.txt"
        COMMAND ${CMAKE_COMMAND} -E echo "-- Interactive: perf report -i ${URLPARSER_PROFILE_OUT}/perf.data"
        WORKING_DIRECTORY ${URLPARSER_PROFILE_OUT}
        USES_TERMINAL
    )
    add_dependencies(urlparser_profile urlparser_benchmark)
else()
    add_custom_target(urlparser_profile
        COMMAND ${CMAKE_COMMAND} -E echo "perf not found - install linux-tools (Linux) to enable 'urlparser_profile'. Falling back to timing-only benchmark:"
        COMMAND $<TARGET_FILE:urlparser_benchmark>
        USES_TERMINAL
    )
    add_dependencies(urlparser_profile urlparser_benchmark)
endif()
