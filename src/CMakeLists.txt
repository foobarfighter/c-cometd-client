#find_package(JsonGlib REQUIRED)
find_package(CURL REQUIRED)

include_directories (${GLIB2_INCLUDE_DIR}
                     ${JsonGlib_INCLUDE_DIRS}
                     ${ev_INCLUDE_DIRS})

add_library(cometd cometd.c
                   error.c
                   conn.c
                   channel.c
                   event.c
                   json.c
                   msg.c
                   http.c
                   inbox.c
                   ext.c
                   transport.c
                   transport_long_polling.c
                   loop.c
                   loops/gthread.c)

add_subdirectory(loops)

target_link_libraries(cometd
                      ${GLIB2_LIBRARIES}
                      ${JsonGlib_LIBRARIES}
                      ${CURL_LIBRARIES}
                      ${deps_LIBRARIES})
