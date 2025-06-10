#include <stdlib.h>

#define FILE_NAME_SIZE 1000

typedef enum {
    WRITE=0,
    READ,
    NONE
} Action;

typedef struct FileLocks FileLocks;

FileLocks *CreateFileLocks( size_t size );

void DeleteFileLocks( FileLocks *file_locks_ptr );

int LockFile( FileLocks *file_locks, char *file_name, Action action );