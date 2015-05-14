find_package(GTest REQUIRED)

include_directories(${GTEST_INCLUDE_DIRS})
include_directories(../src)

set(TEST_FILES test_iputility)

set(SRC_FILES ../src/iputility.c ../src/common.c)

foreach(TESTF ${TEST_FILES})
    add_executable(${TESTF} ${SRC_FILES} ${TESTF}.cxx)
    target_link_libraries(${TESTF} ${GTEST_BOTH_LIBRARIES} ${LIBUV_LIBRARIES})
    add_test(${TESTF} ${TESTF})
endforeach(TESTF)