cmake_minimum_required(VERSION 3.25)

# ======================================================================================================================

option(QTCSG_ENABLE_STATIC_CHECKS "Enable static code checking" OFF)

if (NOT QTCSG_ENABLE_STATIC_CHECKS)
    return()
endif()

include(QtCSGVersionNumbers)

# ======================================================================================================================

function(_qtcsg_accept_clang_tidy result_var executable_path)
    message(VERBOSE "Validating ${executable_path}...")

    execute_process(
        COMMAND "${executable_path}" "--version"
        OUTPUT_VARIABLE _version_output OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    string(REPLACE "\n" ";" _version_output "${_version_output}")
    list(GET _version_output 0 _version_string)
    qtcsg_parse_version(_version)

    if (_version VERSION_EQUAL CLAZY_LLVM_VERSION)
        set("${result_var}" TRUE PARENT_SCOPE)
    else()
        set("${result_var}" FALSE PARENT_SCOPE)
    endif()
endfunction()

# ======================================================================================================================

message(CHECK_START "Looking for Clazy static code analyzer")

find_program( # ----------------------------------------------------------------------- find clazy-standalone executable
    CLAZY_STANDALONE_EXECUTABLE clazy-standalone
    DOC "Path to the Clazy static code analyzer"
    PATHS /opt/clazy/bin /usr/local/bin /usr/bin
    REQUIRED
)

if (CLAZY_STANDALONE_EXECUTABLE)
    execute_process( # --------------------------------------------------------------------------- resolve clazy version
        COMMAND "${CLAZY_STANDALONE_EXECUTABLE}" "--version"
        OUTPUT_VARIABLE clazy_version_output OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    string(REPLACE "\n" ";" clazy_version_output "${clazy_version_output}")

    set(CLAZY_VERSION_STRING "${clazy_version_output}") # --------------------------------- resolve actual clazy version
    list(FILTER CLAZY_VERSION_STRING INCLUDE REGEX "^clazy")
    qtcsg_parse_version(CLAZY_VERSION)

    list(FILTER clazy_version_output EXCLUDE REGEX "^clazy") # ------------------------------------ resolve LLVM version
    list(GET clazy_version_output 0 CLAZY_LLVM_VERSION_STRING)
    qtcsg_parse_version(CLAZY_LLVM_VERSION)

    message(CHECK_PASS "${CLAZY_STANDALONE_EXECUTABLE} (clazy ${CLAZY_VERSION}, LLVM ${CLAZY_LLVM_VERSION})")
else()
    message(CHECK_FAIL "Not found")
endif()


# ======================================================================================================================

message(CHECK_START "Looking for Clazy's Clang Tidy plugin")

find_library( # ----------------------------------------------------------------------- find clazy plugin for clang-tidy
    CLAZY_CLANG_TIDY_PLUGIN "ClazyClangTidy${CMAKE_SHARED_LIBRARY_SUFFIX}"
    DOC "Path to the Clazy's clang-tidy plugin"
    PATHS /opt/clazy/lib /usr/local/lib /usr/lib
    REQUIRED
)

if (CLAZY_CLANG_TIDY_PLUGIN)
    message(CHECK_PASS "${CLAZY_CLANG_TIDY_PLUGIN}")
else()
    message(CHECK_FAIL "Not found")
endif()

# ======================================================================================================================

message(CHECK_START "Looking for Clang Tidy static code analyzer")

find_program( # ----------------------------------------------------------------------------- find clang-tidy executable
    CLANG_TIDY_EXECUTABLE clang-tidy
    DOC "Path to clang's static code analyzer"
    NAMES clang-tidy-${CLAZY_LLVM_VERSION_MAJOR}
    PATHS /opt/llvm/bin /opt/llvm-${CLAZY_LLVM_VERSION}/bin /usr/local/bin /usr/bin
    VALIDATOR _qtcsg_accept_clang_tidy
    REQUIRED
)

if (CLANG_TIDY_EXECUTABLE)
    execute_process( # ---------------------------------------------------------------------- resolve clang-tidy version
        COMMAND "${CLANG_TIDY_EXECUTABLE}" "--version"
        OUTPUT_VARIABLE clang_tidy_version_output OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    string(REPLACE "\n" ";" clang_tidy_version_output "${clang_tidy_version_output}")
    list(GET clang_tidy_version_output 0 CLANG_TIDY_VERSION_STRING)
    qtcsg_parse_version(CLANG_TIDY_VERSION)

    message(CHECK_PASS "${CLANG_TIDY_EXECUTABLE} (${CLANG_TIDY_VERSION})") # ---------------------------- report success
else()
    message(CHECK_FAIL "Not found")
endif()

# ======================================================================================================================

if (NOT CLANG_TIDY_EXECUTABLE
        OR NOT CLAZY_STANDALONE_EXECUTABLE
        OR NOT CLAZY_CLANG_TIDY_PLUGIN)
    message(FATAL_ERROR "Static code checking enabled, but could not find all required tools")
    return()
endif()

if (NOT CLANG_TIDY_VERSION VERSION_EQUAL CLAZY_LLVM_VERSION)
    message(
        FATAL_ERROR "Incorrect Clang Tidy version (${CLANG_TIDY_VERSION_STRING})"
        " for use with the selected Clazy plugin (${CLAZY_LLVM_VERSION_STRING})"
    )
    return()
endif()

# ======================================================================================================================

if (NOT CLAZY_CHECKS) # ---------------------------------------------------------------------------- enable clazy checks
    set(CLAZY_CHECKS "level1")
endif()

if (CLAZY_CHECKS MATCHES "level[0-9]+")
    execute_process(
        COMMAND "${CLAZY_STANDALONE_EXECUTABLE}" "--list-checks" "--checks=${CLAZY_CHECKS}"
        OUTPUT_VARIABLE CLAZY_CHECKS OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    string(REPLACE "\n" ";" CLAZY_CHECKS "${CLAZY_CHECKS}")
    list(FILTER CLAZY_CHECKS INCLUDE REGEX "^[ \t]")
    list(TRANSFORM CLAZY_CHECKS STRIP)
endif()

message(VERBOSE "Selected Clazy checks: ${CLAZY_CHECKS}")
list(TRANSFORM CLAZY_CHECKS PREPEND "clazy-")

# ======================================================================================================================

string(
    JOIN "," CLANG_TIDY_CHECKS
    bugprone-*
    modernize-*
    -modernize-use-trailing-return-type
    ${CLAZY_CHECKS}
)

string(
    JOIN "," CLANG_TIDY_ERRORS
    bugprone-narrowing-conversions
)

set(CMAKE_CXX_CLANG_TIDY
    "${CLANG_TIDY_EXECUTABLE}"
    "--load=${CLAZY_CLANG_TIDY_PLUGIN}"
    "--checks=${CLANG_TIDY_CHECKS}"
    "--warnings-as-errors=${CLANG_TIDY_ERRORS}"
)
