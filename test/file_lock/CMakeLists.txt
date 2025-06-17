add_executable( FileLockTest test.cpp )

target_link_libraries( FileLockTest PRIVATE Threads::Threads file_lock_lib gtest gtest_main )

gtest_discover_tests( FileLockTest )

add_test(NAME FileLockTest_Valgrind
        COMMAND valgrind --leak-check=full --error-exitcode=1 ${CMAKE_CURRENT_BINARY_DIR}/FileLockTest
)