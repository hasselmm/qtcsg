add_executable(QtCSGTest qtcsgtest.cpp qtcsgtest.h)
target_link_libraries(QtCSGTest PRIVATE QtCSG Qt::Test)
add_test(NAME QtCSGTest COMMAND $<TARGET_FILE:QtCSGTest>)
