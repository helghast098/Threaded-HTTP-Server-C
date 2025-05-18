set(HEADER_LISTS "${HTTPServerP_SOURCE_DIR}/include/Queue/queue.h")
add_library(queue STATIC queue.c ${HEADER_LISTS})
target_include_directories(queue PRIVATE ${HTTPServerP_SOURCE_DIR}/include)
