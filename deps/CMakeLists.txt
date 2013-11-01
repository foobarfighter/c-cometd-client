add_subdirectory(ev)
add_library(deps deps.c)
target_link_libraries(deps ev)
