aux_source_directory(. HTTP_SRC)
add_library(HTTP_LIB ${HTTP_SRC})
target_link_libraries(HTTP_LIB BUFFER_LIB LOGGER_LIB)