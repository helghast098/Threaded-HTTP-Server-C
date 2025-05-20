set(HEADER_LISTS "${HTTPServerP_SOURCE_DIR}/include/request_parser/request_parser.h")
add_library(requestparser STATIC request_parser.c ${HEADER_LISTS})
target_include_directories(requestparser PUBLIC ${HTTPServerP_SOURCE_DIR}/include)
