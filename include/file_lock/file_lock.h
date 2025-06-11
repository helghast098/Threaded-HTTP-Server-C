#include <stdlib.h>

#define FILE_NAME_SIZE 1000 // Including null

typedef enum {
    WRITE=0,
    READ,
    NONE
} Action;

typedef struct Link * File_Link;

typedef struct FileLocks FileLocks;

FileLocks *CreateFileLocks( size_t size );

void DeleteFileLocks( FileLocks *file_locks_ptr );

File_Link LockFile( FileLocks *file_locks, char *file_name, Action action );

void UnlockFile( FileLocks *file_locks, File_Link *file_link_ptr );