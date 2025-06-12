
extern "C" {
#include "file_lock/file_lock.h"
}
#include "gtest/gtest.h"

#include <thread>


TEST( FileLockTest, _Creating_And_Destruction ) {
    FileLocks *file_locks = CreateFileLocks( 5 );

    ASSERT_TRUE( file_locks != nullptr );

    DeleteFileLocks( &file_locks );

    ASSERT_TRUE( file_locks == nullptr );

}