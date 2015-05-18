set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -lresolv)

find_package(LibConfig REQUIRED)
include_directories(${LIBCONFIG_INCLUDE_DIR})
add_executable(gdns main.c server.c session.c task.c iputility.c common.c config.c proxy.h proxy.c)
target_link_libraries(gdns ${LIBUV_LIBRARIES} ${LIBCONFIG_LIBRARY})