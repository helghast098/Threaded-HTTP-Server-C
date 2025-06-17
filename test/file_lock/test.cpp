
extern "C" {
#include "file_lock/file_lock.h"
}
#include "gtest/gtest.h"

#include <vector>
#include <thread>
#include <string>
#include <unistd.h>
#include <ctime>
#include <cstdlib>

void lock_test( FileLocks *file_lock, const std::string &file_name ) {
    int time_to_sleep = std::rand() % 5000000;
    Action action = ( time_to_sleep % 2 == 0 ) ? WRITE : READ ;
    FileLink l1 = LockFile(  file_lock, file_name.c_str(), action );

    //usleep( time_to_sleep );

    UnlockFile( file_lock, &l1 );
    
}

TEST( FileLockTest, _Creating_And_Destruction ) {
    FileLocks *file_locks = CreateFileLocks( 5 );

    ASSERT_TRUE( file_locks != nullptr );

    DeleteFileLocks( &file_locks );

    ASSERT_TRUE( file_locks == nullptr );
}

TEST( FileLockTest, _LockSameFileMultipleTimes ) {
    std::vector< std::thread > threads;
    std::srand( std::time({}) );
    FileLocks *file_locks = CreateFileLocks( 10 );
    std::string s = "asdasdasdasdasdasdsdsdsdas.txt";

    for ( int i = 0; i < 5; ++i) {
        threads.push_back( std::thread( lock_test, file_locks, s ) );
    }

    for ( std::thread &th: threads ) {
        th.join();
    }

    DeleteFileLocks( &file_locks );
    
}

TEST ( FileLockTest, _LockingMoreFilesThanQueue ) {
    std::vector< std::thread > threads;
    std::srand( std::time({}) );
    FileLocks *file_locks = CreateFileLocks( 1 );
    std::vector< std::string > strings = {
        "asdasdasdasdasdasdsdsdsdas.txt",
        "aasdasdasdasdasdsdsdsdas.txt",
        "asddasdsdsdsdas.txt",
        "asdasdasdasdasd.txt",
        "asdasdasdaqweqweasdasd.txt"
    };
    
    for ( int i = 0; i < 5; ++i) {
        threads.push_back( std::thread( lock_test, file_locks, strings[ i ] ) );
    }

    for ( std::thread &th: threads ) {
        th.join();
    }

    DeleteFileLocks( &file_locks );

}