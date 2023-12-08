add_library(
    QtCSG
    qtcsg.cpp
    qtcsg.h
    qtcsgio.cpp
    qtcsgio.h
    qtcsgmath.cpp
    qtcsgmath.h
    qtcsgutils.cpp
    qtcsgutils.h
)

target_include_directories(QtCSG PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(QtCSG PUBLIC Qt::Gui)
