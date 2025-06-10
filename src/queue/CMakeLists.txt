set(HEADER_LISTS "${HTTPServerP_SOURCE_DIR}/include/queue/queue.h")
add_library(queue_thread_safe STATIC queue.c ${HEADER_LISTS})
target_compile_definitions( queue_thread_safe PRIVATE "THREAD_SAFE" )
target_include_directories(queue_thread_safe PUBLIC ${HTTPServerP_SOURCE_DIR}/include)

add_library(queue_default STATIC queue.c ${HEADER_LISTS})
target_include_directories(queue_default PUBLIC ${HTTPServerP_SOURCE_DIR}/include)
