cmake_minimum_required(VERSION 3.18)

project(QtCSG VERSION 0.1 LANGUAGES CXX)
enable_testing()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

option(QTCSG_IGNORE_ERRORS "Ignore errors when building CSG trees")

if (QTCSG_IGNORE_ERRORS)
    add_compile_definitions(QTCSG_IGNORE_ERRORS=1)
endif()

add_compile_definitions(
    QT_DISABLE_DEPRECATED_BEFORE=0x050f00
    QT_NO_CONTEXTLESS_CONNECT=1
)

add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-Werror=missing-declarations>
    -Wall -Wextra
    -Werror=sign-compare
    -Werror=switch
    -Werror=unused
    -Werror=type-limits
)

include(QtCSGDebug)
include(QtCSGEnableAlternateLinker)
qtcsg_enable_alternate_linker()

# Find Qt5 or Qt6
set(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)
set(CMAKE_FIND_PACKAGE_SORT_ORDER NAME)

set(QT_MODULES Core Gui Test Widgets 3DCore 3DExtras 3DRender)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_MODULES})
message(STATUS "Building for Qt ${QT_VERSION} from ${QT_DIR}")
set(Qt${QT_VERSION_MAJOR}_DIR "${QT_DIR}")

find_package(Qt${QT_VERSION_MAJOR} ${QT_VERSION} REQUIRED COMPONENTS ${QT_MODULES})

include(QtCSGBackports)

add_subdirectory(qtcsg)
add_subdirectory(qt3dcsg)

add_subdirectory(demo)
add_subdirectory(tests)

add_custom_target(
    docs SOURCES
    LICENSE.md
    LICENSE.spdx
    licenses/BSD-3-Clause.md
    licenses/GPL-3.0-or-later.md
    README.md
    docs/demo.png
)
add_custom_target(
    github SOURCES
    .github/workflows/autobuild.yml
)
