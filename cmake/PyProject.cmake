# cmake/modules/PyProject.cmake
function(extract_pyproject_info)
    set(filename "${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml")
    set(PyProject_TOML_PATH ${filename} PARENT_SCOPE)

    # Read lines from the file into a list
    file(STRINGS ${filename} Lines)

    # Iterate over the list to find the version line
    foreach(Line ${Lines})
        # Use a regular expression to extract the version number
        if(Line MATCHES "version = \"([0-9]+\\.[0-9]+\\.[0-9]+)\"")
            set(PyProject_VERSION "${CMAKE_MATCH_1}" PARENT_SCOPE)
        endif()

        # Extract the project name
        if(Line MATCHES "name = \"([A-Za-z0-9_-]+)\"")
            set(PyProject_NAME "${CMAKE_MATCH_1}" PARENT_SCOPE)
        endif()

        # Extract the project description
        if(Line MATCHES "description = \"([A-Za-z0-9_ -]+)\"")
            set(PyProject_DESCRIPTION "${CMAKE_MATCH_1}" PARENT_SCOPE)
        endif()
    endforeach()
endfunction()

extract_pyproject_info()