# PublicSuffixList.cmake
#
# Locates the Public Suffix List (.dat) file that gets embedded into
# liburlparser at build time (see EmbedFile.cmake / PUBLIC_SUFFIX_LIST_DAT_H
# in the top-level CMakeLists.txt).
#
# The download URL is *not* hardcoded here. It's read straight out of
# include/urlparser.h:
#   #define URLPARSER_PUBLIC_SUFFIX_LIST_URL "https://..."
# the same way cmake/DynamicVersion.cmake reads the version out of that
# same header - one file stays the single source of truth instead of two.
#
# Caching: the download lands in ${CMAKE_CURRENT_BINARY_DIR}/src/, i.e.
# inside the build directory rather than the source tree. As long as the
# build directory exists, re-running cmake reuses that cached copy and
# never touches the network again. Deleting the build directory (the
# usual `rm -rf build`) is the cache-invalidation mechanism - simple,
# explicit, no extra state file to manage or go stale on its own.
#
# On any download failure (offline, DNS, timeout, HTTP error), this warns
# and falls back to whatever copy is committed in the repository root
# instead of failing the build - a network hiccup shouldn't be fatal. Note
# that fallback copy is only ever refreshed by a successful download, so
# it can drift out of date; the WARNING says so every time it's used.
#
# Public API:
#   fetch_public_suffix_list(
#       HEADER_FILE <path>              # include/urlparser.h
#       [VERSION_PREFIX <prefix>]       # "URLPARSER_" - matches the
#                                        # #define's prefix, same
#                                        # convention as DynamicVersion.cmake
#       [REPO_FALLBACK_FILE <path>]     # committed public_suffix_list.dat,
#                                        # used only if the download fails
#       OUTPUT_VAR <var>                # -> path to the .dat file to embed
#       [URL_OUTPUT_VAR <var>]          # -> the URL read from the header
#       [TIMEOUT <seconds>]             # default 10
#   )

include_guard(GLOBAL)

# Pull URLPARSER_PUBLIC_SUFFIX_LIST_URL "..." out of the header. Same
# line-by-line, anchored-regex approach as DynamicVersion.cmake, for the
# same reason: robust against the value appearing in a comment or string
# elsewhere in the file.
function(_psl_read_url header_file version_prefix out_var)
    if(NOT EXISTS "${header_file}")
        message(FATAL_ERROR "PublicSuffixList: header file not found: ${header_file}")
    endif()

    set(_regex "^[ \t]*#[ \t]*define[ \t]+${version_prefix}PUBLIC_SUFFIX_LIST_URL[ \t]+\"([^\"]*)\"")
    file(STRINGS "${header_file}" _lines REGEX "${_regex}")

    if(NOT _lines)
        message(FATAL_ERROR
            "PublicSuffixList: could not find ${version_prefix}PUBLIC_SUFFIX_LIST_URL "
            "in ${header_file}\n"
            "PublicSuffixList: ensure the header defines:\n"
            "  #define ${version_prefix}PUBLIC_SUFFIX_LIST_URL \"https://...\"\n"
        )
    endif()

    list(GET _lines 0 _line)
    string(REGEX MATCH "${_regex}" _matched "${_line}")
    if(NOT _matched)
        message(FATAL_ERROR "PublicSuffixList: failed to parse URL out of: ${_line}")
    endif()
    set(${out_var} "${CMAKE_MATCH_1}" PARENT_SCOPE)
endfunction()

function(fetch_public_suffix_list)
    set(oneValueArgs HEADER_FILE VERSION_PREFIX REPO_FALLBACK_FILE OUTPUT_VAR URL_OUTPUT_VAR TIMEOUT)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

    if(NOT ARG_HEADER_FILE)
        message(FATAL_ERROR "fetch_public_suffix_list: HEADER_FILE is required")
    endif()
    if(NOT ARG_OUTPUT_VAR)
        message(FATAL_ERROR "fetch_public_suffix_list: OUTPUT_VAR is required")
    endif()
    if(NOT ARG_TIMEOUT)
        set(ARG_TIMEOUT 10)
    endif()

    _psl_read_url("${ARG_HEADER_FILE}" "${ARG_VERSION_PREFIX}" _psl_url)

    if(ARG_URL_OUTPUT_VAR)
        set(${ARG_URL_OUTPUT_VAR} "${_psl_url}" PARENT_SCOPE)
    endif()

    # The cache lives inside the build tree - src/ here means
    # <build-dir>/src/, not the project's own src/. Removing the build
    # directory is what forces a fresh download.
    set(_cache_dir "${CMAKE_CURRENT_BINARY_DIR}/src")
    set(_cache_file "${_cache_dir}/public_suffix_list.dat")

    if(EXISTS "${_cache_file}")
        message(STATUS "[PSL] Using cached download: ${_cache_file}")
        set(${ARG_OUTPUT_VAR} "${_cache_file}" PARENT_SCOPE)
        return()
    endif()

    file(MAKE_DIRECTORY "${_cache_dir}")
    message(STATUS "[PSL] Downloading ${_psl_url}")
    file(DOWNLOAD "${_psl_url}" "${_cache_file}"
         STATUS _dl_status
         TIMEOUT ${ARG_TIMEOUT}
         TLS_VERIFY ON)
    list(GET _dl_status 0 _dl_code)

    if(_dl_code EQUAL 0 AND EXISTS "${_cache_file}")
        file(SIZE "${_cache_file}" _dl_size)
        if(_dl_size GREATER 0)
            message(STATUS "[PSL] Downloaded to ${_cache_file} (${_dl_size} bytes)")
            set(${ARG_OUTPUT_VAR} "${_cache_file}" PARENT_SCOPE)
            return()
        endif()
    endif()

    # Download failed - don't leave a truncated/empty file behind to be
    # mistaken for a valid cache on the next configure.
    list(GET _dl_status 1 _dl_msg)
    file(REMOVE "${_cache_file}")

    if(ARG_REPO_FALLBACK_FILE AND EXISTS "${ARG_REPO_FALLBACK_FILE}")
        message(WARNING
            "[PSL] Failed to download the Public Suffix List from ${_psl_url} "
            "(${_dl_msg}). Falling back to the copy committed in the repo: "
            "${ARG_REPO_FALLBACK_FILE}. That copy is only refreshed by a "
            "successful download, so it may be out of date."
        )
        set(${ARG_OUTPUT_VAR} "${ARG_REPO_FALLBACK_FILE}" PARENT_SCOPE)
    else()
        message(FATAL_ERROR
            "[PSL] Failed to download the Public Suffix List from ${_psl_url} "
            "(${_dl_msg}), and no fallback file was found at "
            "${ARG_REPO_FALLBACK_FILE}."
        )
    endif()
endfunction()
