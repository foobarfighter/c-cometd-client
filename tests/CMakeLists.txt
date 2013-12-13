find_package(Check REQUIRED)

include_directories (${GLIB2_INCLUDE_DIR}
                     ${JsonGlib_INCLUDE_DIRS}
                     ${ev_INCLUDE_DIRS}
                     ${CHECK_INCLUDE_DIRS})

include_directories(. ../src)

add_executable(check_cometd check_cometd_main.c
                            check_cometd.c
                            check_event.c
                            check_msg.c
                            check_conn.c
                            check_transport.c
                            check_channel.c
                            check_cometd_integration.c
                            check_http.c
                            check_inbox.c
                            check_test_helper.c
                            test_helper.c)

add_executable(check_stress check_cometd_json_stress.c)

target_link_libraries(check_cometd cometd deps ${CHECK_LIBRARIES})
target_link_libraries(check_stress cometd deps ${CHECK_LIBRARIES})

add_test(check_cometd ${CMAKE_CURRENT_BINARY_DIR}/check_cometd)
