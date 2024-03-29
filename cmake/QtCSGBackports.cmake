if (NOT QT_VERSION STREQUAL Qt${QT_VERSION_MAJOR}_VERSION)
    message(FATAL_ERROR "Unexpected version for Qt${QT_VERSION_MAJOR} package: ${Qt${QT_VERSION_MAJOR}_VERSION}")
endif()

if (NOT QT_DIR STREQUAL Qt${QT_VERSION_MAJOR}_DIR)
    message(FATAL_ERROR "Unexpected directory for Qt${QT_VERSION_MAJOR} package: ${Qt${QT_VERSION_MAJOR}_DIR}")
endif()

foreach(module ${QT_MODULES})
    if (NOT TARGET Qt::${module})
        add_library(Qt::${module} ALIAS Qt${QT_VERSION_MAJOR}::${module})
    endif()
endforeach()

# Very minimal backport of qt_add_executable for Qt
if (NOT COMMAND qt_add_executable)
    function(qt_add_executable NAME)
        if (ANDROID)
            add_library(${NAME} SHARED ${ARGN})
        else()
            add_executable(${NAME} ${ARGN})
        endif()
    endfunction()
endif()
