set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -lresolv)
add_executable(gdns main.c server.c session.c task.c iputility.c common.c)
target_link_libraries(gdns ${LIBUV_LIBRARIES})