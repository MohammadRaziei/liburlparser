# EmbedFile.cmake
#
# Embeds a data file into a C++ header as `inline const std::string`,
# generated via configure_file() -> runs at CONFIGURE time (like
# libmbtiles' EmbededTemplate.cmake), so the header exists right after
# `cmake -B build`, not only after the first `cmake --build`.
#
# Unlike the classic `R"tag(...)tag"` raw-string trick used for small
# templates, this splits the file into one escaped string literal PER LINE
# and relies on the compiler's adjacent-string-literal concatenation
# ("a" "b" == "ab"). Each individual literal therefore stays far below
# MSVC's per-literal length limit (a single raw string or one giant literal
# both choke MSVC on large files), while remaining plain portable C++ -
# no compiler extensions needed.
#
# Usage:
#   include(${CMAKE_CURRENT_LIST_DIR}/EmbedFile.cmake)
#   embed_file_as_string(<input-file> <output-header> <namespace> <var-name>)
#
# Produces (in OUTPUT):
#   namespace <namespace> {
#       inline const std::string <var-name> = "line1\n" "line2\n" ... ;
#   }

set(_EMBED_FILE_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(embed_file_as_string INPUT OUTPUT NAMESPACE VAR_NAME)
    set(EMBED_TEMPLATE_HEADER_IN ${_EMBED_FILE_MODULE_DIR}/utils/embedded_string.in.h)

    string(TOUPPER "${VAR_NAME}" _GUARD_UPPER)
    set(INCLUDE_GUARD "LIBURLPARSER_EMBEDDED_${_GUARD_UPPER}_H")

    file(READ "${INPUT}" _RAW_CONTENT)

    # Normalize line endings so we don't embed stray \r.
    string(REPLACE "\r\n" "\n" _RAW_CONTENT "${_RAW_CONTENT}")

    # Escape backslashes and double quotes first (order matters).
    string(REPLACE "\\" "\\\\" _RAW_CONTENT "${_RAW_CONTENT}")
    string(REPLACE "\"" "\\\"" _RAW_CONTENT "${_RAW_CONTENT}")

    # One string literal per source line: "...\n"
    # then let the compiler concatenate them.
    string(REPLACE "\n" "\\n\"\n    \"" _RAW_CONTENT "${_RAW_CONTENT}")

    set(VAR "${VAR_NAME}")
    set(CONTENT "    \"${_RAW_CONTENT}\"")

    get_filename_component(_OUTDIR "${OUTPUT}" DIRECTORY)
    file(MAKE_DIRECTORY "${_OUTDIR}")

    configure_file("${EMBED_TEMPLATE_HEADER_IN}" "${OUTPUT}" @ONLY)

    # Make sure a re-configure (and therefore regeneration) is triggered
    # whenever the source data file changes.
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${INPUT}")
endfunction()
