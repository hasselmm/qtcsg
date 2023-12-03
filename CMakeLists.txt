cmake_minimum_required(VERSION 3.14)

project(QtCSG VERSION 0.1 LANGUAGES CXX)
enable_testing()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x050f00)
add_compile_options(-Werror=switch -Werror=unused)

# Find Qt5 or Qt6
set(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)
set(CMAKE_FIND_PACKAGE_SORT_ORDER NAME)

set(QT_MODULES Core Gui Test Widgets 3DCore 3DExtras 3DRender)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_MODULES})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_MODULES})

foreach(module ${QT_MODULES})
    add_library(Qt::${module} ALIAS Qt${QT_VERSION_MAJOR}::${module})
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

add_subdirectory(qtcsg)
add_subdirectory(qt3dcsg)

add_subdirectory(demo)
add_subdirectory(tests)

add_custom_target(docs SOURCES README.md docs/demo.png)
add_custom_target(github SOURCES .github/workflows/autobuild.yml)
