# CreateDocs.cmake — Route Index Page
# Called via: cmake -P CreateDocs.cmake
# Required CMake variables (passed via -D):
#   OUTPUT_DIR, INDEX_IN, PROJECT_VERSION, LOGO_SQ, CSS_FILE

cmake_minimum_required(VERSION 3.19)

file(MAKE_DIRECTORY "${OUTPUT_DIR}")

if(DEFINED CSS_FILE)
  file(COPY_FILE "${CSS_FILE}" "${OUTPUT_DIR}/urlparser-docs.css")
endif()

# ── Read SVG logo for embedding ───────────────────────────────
if(DEFINED LOGO_SQ AND EXISTS "${LOGO_SQ}")
  file(READ "${LOGO_SQ}" LOGO_CONTENT)
endif()

# ── Copy logo (also used as favicon) ──────────────────────────
if(DEFINED LOGO_SQ)
  file(COPY_FILE "${LOGO_SQ}" "${OUTPUT_DIR}/urlparser-sq.svg")
endif()

# ── Generate index.html ───────────────────────────────────────
file(READ "${INDEX_IN}" INDEX_CONTENT)
string(REPLACE "@PROJECT_VERSION@" "${PROJECT_VERSION}" INDEX_CONTENT "${INDEX_CONTENT}")
string(REPLACE "@LOGO_CONTENT@"    "${LOGO_CONTENT}"    INDEX_CONTENT "${INDEX_CONTENT}")
string(REPLACE "@FAVICON@"         "urlparser-sq.svg"   INDEX_CONTENT "${INDEX_CONTENT}")
file(WRITE "${OUTPUT_DIR}/index.html" "${INDEX_CONTENT}")

message(STATUS "-- Route index created at: ${OUTPUT_DIR}/index.html")
