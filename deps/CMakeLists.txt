add_subdirectory(ev)
add_subdirectory(http-parser)

add_library(deps deps.c)
target_link_libraries(deps ev http_parser)
