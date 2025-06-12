set(HEADER_LISTS "${HTTPServerP_SOURCE_DIR}/include/file_lock/file_lock.h")
add_library( file_lock_lib STATIC file_lock.c ${HEADER_LISTS} )
target_include_directories( file_lock_lib PUBLIC ${HTTPServerP_SOURCE_DIR}/include )
target_link_libraries( file_lock_lib PRIVATE queue_default Threads::Threads)