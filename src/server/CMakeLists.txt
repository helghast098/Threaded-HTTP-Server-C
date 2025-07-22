set( LIBRARIES  )

add_executable(httpserver server.c)
set_target_properties(httpserver PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/httpserver)
target_include_directories(httpserver PRIVATE ${HTTPServerP_SOURCE_DIR}/include)
target_link_libraries(httpserver PRIVATE Threads::Threads
                                 PRIVATE bind
                                 PRIVATE file_lock_lib
                                 PRIVATE httpmethods
                                 PRIVATE queue_thread_safe
                                 PRIVATE requestparser
)
