find_package(Check REQUIRED)
include_directories (${GLIB2_INCLUDE_DIR} ${JsonGlib_INCLUDE_DIRS} ${ev_INCLUDE_DIRS} ${CHECK_INCLUDE_DIRS})
include_directories(. ../src)

add_executable(check_cometd check_cometd_main.c
                            check_cometd.c
                            check_http.c
                            test_helper.c)

target_link_libraries(check_cometd cometd deps /opt/local/lib/libcheck.dylib )
add_test(check_cometd ${CMAKE_CURRENT_BINARY_DIR}/check_cometd)
