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
# Caching: the download lands directly in the build tree at
# ${CMAKE_CURRENT_BINARY_DIR}/public_suffix_list.dat, and that file's own
# existence *is* the cache - if it's there, a download already succeeded
# for this build directory and TARGET_FILE was already written back then,
# so we skip the network and do nothing further. Deleting the build
# directory removes that cached copy and forces a fresh download (and a
# fresh copy into TARGET_FILE) on the next configure.
#
# TARGET_FILE (src/public_suffix_list.dat, tracked in git) is the one
# real copy used for embedding and doubles as the offline fallback.
#
# On any download failure (offline, DNS, timeout, HTTP error), this warns
# and keeps using the existing TARGET_FILE instead of failing the build -
# a network hiccup shouldn't be fatal. No cache file is left behind on
# failure, so the very next configure retries the download rather than
# silently treating the stale file as fresh.
#
# Public API:
#   fetch_public_suffix_list(
#       HEADER_FILE <path>              # include/urlparser.h
#       [VERSION_PREFIX <prefix>]       # "URLPARSER_" - matches the
#                                        # #define's prefix, same
#                                        # convention as DynamicVersion.cmake
#       TARGET_FILE <path>              # the one real, tracked .dat file -
#                                        # overwritten on every successful
#                                        # download, used as-is on failure
#       OUTPUT_VAR <var>                # -> path to the .dat file to embed
#                                        # (always == TARGET_FILE)
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
    set(oneValueArgs HEADER_FILE VERSION_PREFIX TARGET_FILE OUTPUT_VAR URL_OUTPUT_VAR TIMEOUT)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

    if(NOT ARG_HEADER_FILE)
        message(FATAL_ERROR "fetch_public_suffix_list: HEADER_FILE is required")
    endif()
    if(NOT ARG_TARGET_FILE)
        message(FATAL_ERROR "fetch_public_suffix_list: TARGET_FILE is required")
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

    # ARG_TARGET_FILE (src/public_suffix_list.dat) is the one real, tracked
    # copy, used for embedding whether we downloaded this run or not - so
    # OUTPUT_VAR is always just ARG_TARGET_FILE, set once at the end.
    #
    # The cache is the downloaded file itself, sitting in the build tree
    # at _cache_file. Its existence *is* the "already downloaded for this
    # build directory" signal, so if it's there we skip the network
    # entirely - no re-copy, TARGET_FILE was already written the run this
    # file first appeared. Deleting the build directory removes the cache
    # and forces a fresh download on the next configure.
    set(_cache_file "${CMAKE_CURRENT_BINARY_DIR}/public_suffix_list.dat")

    if(NOT EXISTS "${_cache_file}")
        message(STATUS "[PSL] Downloading ${_psl_url}")
        file(DOWNLOAD "${_psl_url}" "${_cache_file}"
             STATUS _dl_status
             TIMEOUT ${ARG_TIMEOUT}
             TLS_VERIFY ON)
        list(GET _dl_status 0 _dl_code)
        list(GET _dl_status 1 _dl_msg)

        set(_dl_size 0)
        if(_dl_code EQUAL 0 AND EXISTS "${_cache_file}")
            file(SIZE "${_cache_file}" _dl_size)
        endif()

        if(_dl_code EQUAL 0 AND _dl_size GREATER 0)
            # Overwrite the real, tracked file in place - this is the
            # "update src/public_suffix_list.dat every time" part.
            # configure_file(... COPYONLY) is used rather than
            # file(RENAME) because RENAME is a plain filesystem move and
            # fails with "cannot move to a different disk drive" when the
            # build dir and the source tree sit on different drives (seen
            # on Windows CI, where cibuildwheel builds under a C: temp
            # dir while the checkout is on D:). COPYONLY copies actual
            # content, so it works across drives/filesystems, and has
            # worked since ancient CMake versions.
            configure_file("${_cache_file}" "${ARG_TARGET_FILE}" COPYONLY)
            message(STATUS "[PSL] Downloaded and updated ${ARG_TARGET_FILE} (${_dl_size} bytes)")
        else()
            # Download failed - don't leave a truncated/empty file behind
            # in the build dir, or it'd be mistaken for a valid cache on
            # the next configure and silently skip retrying.
            file(REMOVE "${_cache_file}")

            if(EXISTS "${ARG_TARGET_FILE}")
                message(WARNING
                    "[PSL] Failed to download the Public Suffix List from ${_psl_url} "
                    "(${_dl_msg}). Using the existing ${ARG_TARGET_FILE} as-is - it "
                    "is only refreshed by a successful download, so it may be out "
                    "of date."
                )
            else()
                message(FATAL_ERROR
                    "[PSL] Failed to download the Public Suffix List from ${_psl_url} "
                    "(${_dl_msg}), and no existing file was found at "
                    "${ARG_TARGET_FILE}."
                )
            endif()
        endif()
    else()
        message(STATUS "[PSL] Already downloaded for this build dir - using ${ARG_TARGET_FILE}")
    endif()

    set(${ARG_OUTPUT_VAR} "${ARG_TARGET_FILE}" PARENT_SCOPE)
endfunction()
