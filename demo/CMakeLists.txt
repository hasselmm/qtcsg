qt_add_executable(
    QtCSGDemo
    qtcsgdemo.cpp
    qtcsgdemo.qrc

    orbitalcameracontroller.cpp
    orbitalcameracontroller.h
    wireframematerial.cpp
    wireframematerial.h
)

target_link_libraries(QtCSGDemo PRIVATE Qt3DCSG Qt::3DExtras Qt::Widgets)

#if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
## Define target properties for Android with Qt 6 as:
##    set_property(TARGET QtCSG APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
##                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
## For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
## Define properties for Android with Qt 5 after find_package() calls as:
##    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#endif()

set_target_properties(QtCSGDemo PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER qtcsg.taschenorakel.de
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
