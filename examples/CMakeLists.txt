include_directories (${GLIB2_INCLUDE_DIR}
                     ${JsonGlib_INCLUDE_DIRS})

include_directories(. ../src)
add_executable(basic basic.c)
target_link_libraries(basic cometd deps)
