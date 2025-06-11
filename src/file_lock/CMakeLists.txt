set(HEADER_LISTS "${HTTPServerP_SOURCE_DIR}/include/")
add_library( file_lock_lib file_lock.c ${HEADER_LISTS} )
target_link_libraries( file_lock_lib PRIVATE queue_default Threads::Threads)
target_include_directories( file_lock_lib PUBLIC ${HTTPServerP_SOURCE_DIR}/include )