add_executable( FileLockTest test.cpp )

target_link_libraries( FileLockTest PRIVATE Threads::Threads file_lock_lib gtest gtest_main )

gtest_discover_tests( FileLockTest )