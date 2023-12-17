qt_add_library(
    QtCSGAppSupport STATIC
    qtcsgappsupport.cpp
    qtcsgappsupport.h
    orbitcameracontroller.cpp
    orbitcameracontroller.h
    wireframematerial.cpp
    wireframematerial.h
    wireframematerial.qrc
)

target_link_libraries(QtCSGAppSupport PUBLIC Qt3DCSG Qt::3DExtras)

qt_add_executable(QtCSGDemo qtcsgdemo.cpp)
target_link_libraries(QtCSGDemo PRIVATE QtCSGAppSupport Qt::Widgets)

set_target_properties(QtCSGDemo PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER qtcsgdemo.taschenorakel.de
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

#if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
## Define target properties for Android with Qt 6 as:
##    set_property(TARGET QtCSG APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
##                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
## For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
## Define properties for Android with Qt 5 after find_package() calls as:
##    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#endif()
