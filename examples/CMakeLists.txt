include_directories (${GLIB2_INCLUDE_DIR}
                     ${JsonGlib_INCLUDE_DIRS})

include_directories(. ../include)

add_executable(basic basic.c)
target_link_libraries(basic cometd deps)

add_executable(yammer yammer.c)
target_link_libraries(yammer cometd deps)

add_executable(stress stress.c)
target_link_libraries(stress cometd deps)
