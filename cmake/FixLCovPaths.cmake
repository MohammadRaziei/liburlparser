# Read the content of the coverage file
file(READ ${COV_FILE} CONTENT)

# Replace the absolute source path with an empty string
# To relative paths like: include/...
string(REPLACE "SF:${SOURCE_PATH}" "SF:${PREFIX_PATH}" CONTENT "${CONTENT}")

# coverage.py writes a function record (FN/FNDA) for every Python def. The def line runs when the module is
# imported, so a function that is never called has FNDA:0 next to a hit DA line, which lcov 2.x rejects as
# inconsistent ("function is not hit but line is hit") and aborts the merge. Python function coverage adds
# nothing over line coverage, so STRIP_FUNCTIONS=ON drops the records (and their FNF/FNH totals).
if(STRIP_FUNCTIONS)
    string(REGEX REPLACE "(^|\n)(FN|FNDA|FNF|FNH):[^\n]*" "\\1" CONTENT "${CONTENT}")
    string(REGEX REPLACE "\n\n+" "\n" CONTENT "${CONTENT}")
endif()

# Write the modified content back to the file
file(WRITE ${COV_FILE} "${CONTENT}")
