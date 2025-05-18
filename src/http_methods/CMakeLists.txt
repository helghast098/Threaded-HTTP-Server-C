set(HEADER_LISTS "${HTTPServerP_SOURCE_DIR}/include/HTTP-Methods/http_methods.h")
add_library(httpmethods STATIC http_methods.c ${HEADER_LISTS})
target_include_directories(httpmethods PRIVATE ${HTTPServerP_SOURCE_DIR}/include)
