# qtcsg_parse_version(<output_variable> [<version_string>])
#
# Extracts a normalized dotted version number from a string and populates
# version components into well‑known variables.
#
# This function accepts an optional version string. If no argument is given,
# it attempts to derive the version from:
#
#   <output_variable>_STRING
#   <output_variable>
#
# in that order. The first non‑empty value is used as the source string.
#
# From the selected string, the function extracts the first occurrence of a
# version‑like pattern of the form:
#
#     <digits>(.<digits>)*
#
# The match must be either at the beginning of the string or preceded by a
# non‑digit character. If no valid version can be found, the function aborts
# with a fatal error.
#
# On success, the following variables are set in the parent scope:
#
#   <output_variable>          — the normalized version string (e.g. "1.2.3")
#   <output_variable>_MAJOR    — first numeric component
#   <output_variable>_MINOR    — second component, if present
#   <output_variable>_PATCH    — third component, if present
#   <output_variable>_TWEAK    — fourth component, if present
#
# Versions with more than four components are truncated.
#
# Example:
#
#   qtcsg_parse_version(MYLIB_VERSION "v1.4.2-beta")
#
# This produces:
#
#   MYLIB_VERSION        = "1.4.2"
#   MYLIB_VERSION_MAJOR  = "1"
#   MYLIB_VERSION_MINOR  = "4"
#   MYLIB_VERSION_PATCH  = "2"
#
function(qtcsg_parse_version output_variable)
    # get version string from optional argument
    if (NOT "${ARGN}" STREQUAL "")
        set(_version_string "${ARGN}")
    endif()

    # find version string via output_variable if no argument was specified
    if (NOT DEFINED _version_string OR _version_string STREQUAL "")
        set(_version_string "${${output_variable}_STRING}")
    endif()

    if (NOT DEFINED _version_string OR _version_string STREQUAL "")
        set(_version_string "${${output_variable}}")
    endif()

    # extract version number from version string
    string(REGEX MATCH "(^|[^0-9])(([0-9]+\\.)*[0-9]+)" _version_found "${_version_string}")

    if (_version_found STREQUAL "")
        message(FATAL_ERROR "Version string argument required, or valid version number in ${output_variable}")
    endif()

    set(_version_number "${CMAKE_MATCH_2}")
    set("${output_variable}" "${_version_number}" PARENT_SCOPE)

    # extract known version components into common variables
    string(REPLACE "." ";" _version_parts "${_version_number}")

    set(_part_names MAJOR MINOR PATCH TWEAK)
    list(LENGTH _version_parts _version_length)

    if (_version_length GREATER 4)
        set(_version_length 4)
    endif()

    math(EXPR _last_part "${_version_length} - 1")

    foreach(_index RANGE "${_last_part}")
        list(GET _version_parts "${_index}" _value)
        list(GET _part_names "${_index}" _name)
        set("${output_variable}_${_name}" "${_value}" PARENT_SCOPE)
    endforeach()
endfunction()
