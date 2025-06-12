add_executable( QueueTest test.cpp )
target_link_libraries( QueueTest PRIVATE Threads::Threads queue_thread_safe gtest gtest_main)
gtest_discover_tests( QueueTest )