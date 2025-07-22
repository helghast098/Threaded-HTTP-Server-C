set(HEADER_LISTS "${HTTPServerP_SOURCE_DIR}/include/bind/bind.h")
add_library(bind STATIC bind.c ${HEADER_LISTS})
target_include_directories(bind PRIVATE ${HTTPServerP_SOURCE_DIR}/include)
