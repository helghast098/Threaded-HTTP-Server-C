set(HEADER_LISTS "${HTTPServerP_SOURCE_DIR}/include/queue/queue.h")
add_library(queue STATIC queue.c ${HEADER_LISTS})
target_include_directories(queue PUBLIC ${HTTPServerP_SOURCE_DIR}/include)
