find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(../src)

set(TEST_FILES testServerConfig)

set(SRC_FILES ../src/ServerConfig.cpp ../src/Proxy.cpp)

foreach(TESTF ${TEST_FILES})
    add_executable(${TESTF} ${SRC_FILES} ${TESTF}.cpp)
    target_link_libraries(${TESTF} ${GTEST_BOTH_LIBRARIES} ${LIBCONFIGPP_LIBRARY} pthread)
    add_test(${TESTF} ${TESTF})
endforeach(TESTF)

file(COPY "gdns.conf" "subnets.txt" DESTINATION ./)