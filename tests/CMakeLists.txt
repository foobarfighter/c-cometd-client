find_package(Check REQUIRED)

include_directories (${GLIB2_INCLUDE_DIR}
                     ${JsonGlib_INCLUDE_DIRS}
                     ${ev_INCLUDE_DIRS}
                     ${CHECK_INCLUDE_DIRS})

include_directories(. ../include)

add_executable(check_cometd check_cometd_main.c
                            check_cometd.c
                            check_event.c
                            check_msg.c
                            check_error.c
                            check_conn.c
                            check_transport.c
                            check_channel.c
                            check_cometd_integration.c
                            check_http.c
                            check_inbox.c
                            check_test_helper.c
                            check_ext.c
                            test_helper.c)

target_link_libraries(check_cometd cometd deps ${CHECK_LIBRARIES})

add_test(check_cometd ${CMAKE_CURRENT_BINARY_DIR}/check_cometd)
