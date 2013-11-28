#find_package(JsonGlib REQUIRED)
find_package(CURL REQUIRED)

include_directories (${GLIB2_INCLUDE_DIR}
                     ${JsonGlib_INCLUDE_DIRS}
                     ${ev_INCLUDE_DIRS})

add_library(cometd cometd.c
                   cometd_json.c
                   cometd_msg.c
                   http.c
                   transport_long_polling.c)

target_link_libraries(cometd
                      ${GLIB2_LIBRARIES}
                      ${JsonGlib_LIBRARIES}
                      ${CURL_LIBRARIES}
                      ${deps_LIBRARIES})
