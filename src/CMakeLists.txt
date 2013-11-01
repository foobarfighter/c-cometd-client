include_directories (${GLIB2_INCLUDE_DIR} ${JsonGlib_INCLUDE_DIRS} ${ev_INCLUDE_DIRS})
add_library(cometd cometd.c)
target_link_libraries(cometd GLIB2 JsonGlib ev)
