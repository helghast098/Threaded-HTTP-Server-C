add_executable( QueueTest test.cpp )
target_link_libraries( QueueTest PRIVATE  queue gtest gtest_main)
gtest_discover_tests( QueueTest )