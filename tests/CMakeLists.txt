function(qtcsg_add_testsuite NAME)
    add_executable(${NAME} ${SOURCE} ${ARGN})
    target_link_libraries(${NAME} PRIVATE QtCSGTestSuite)
    add_test(NAME ${NAME} COMMAND $<TARGET_FILE:${NAME}>)
endfunction()

add_library(QtCSGTestSuite qtcsgtest.h)
target_link_libraries(QtCSGTestSuite PUBLIC QtCSG Qt::Test)

qtcsg_add_testsuite(QtCSGTest qtcsgtest.cpp)
qtcsg_add_testsuite(QtCSGMathTest qtcsgmathtest.cpp)
