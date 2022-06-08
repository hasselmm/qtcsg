add_library(Qt3DCSG qt3dcsg.cpp qt3dcsg.h)
target_include_directories(Qt3DCSG PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(Qt3DCSG PUBLIC QtCSG Qt::3DRender)
