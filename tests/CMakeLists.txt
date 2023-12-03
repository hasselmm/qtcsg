function(qtcsg_add_testsuite NAME)
    set(options)
    set(multivalue SOURCES LIBRARIES)
    cmake_parse_arguments(TESTSUITE "${options}" "${onevalue}" "${multivalue}" ${ARGN})
    message(STATUS: "${NAME}: TESTSUITE_LIBRARIES: ${TESTSUITE_LIBRARIES}")

    qt_add_executable(${NAME} ${TESTSUITE_SOURCES})
    target_link_libraries(${NAME} PRIVATE QtCSGTestSuite ${TESTSUITE_LIBRARIES})
    add_test(NAME ${NAME} COMMAND $<TARGET_FILE:${NAME}>)
endfunction()

add_library(QtCSGTestSuite OBJECT qtcsgtest.h)
target_link_libraries(QtCSGTestSuite PUBLIC QtCSG Qt::Test)

qtcsg_add_testsuite(QtCSGTest SOURCES qtcsgtest.cpp LIBRARIES QtCSGAssets)
qtcsg_add_testsuite(QtCSGIOTest SOURCES qtcsgiotest.cpp)
qtcsg_add_testsuite(QtCSGMathTest SOURCES qtcsgmathtest.cpp)
