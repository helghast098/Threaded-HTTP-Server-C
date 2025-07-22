#ifndef FILE_LOCK_H
#define FILE_LOCK_H

#include <stdlib.h>

#define FILE_NAME_SIZE 1000 // Including null

typedef enum {
    WRITE=0,
    READ,
    NONE
} Action;

typedef struct Link * FileLink; // Returned by LockFile to be used by UnlockFile

typedef struct FileLocks FileLocks;

/**
 * @brief Creates file lock data struct of size size
 * @param size: the number of files to handle locks for
 * @return pointer to the FileLock data structure
 */
FileLocks *CreateFileLocks( size_t size );

/**
 * @note call only once
 * @brief deletes the FileLock data structure make
 * @param file_locks_ptr - is set to null when ran
 */
void DeleteFileLocks( FileLocks **file_locks_ptr );

/**
 * @brief locks the file
 * @param file_locks :lock of the files
 * @param file_name :name of file. MUST BE NULL TERMINATED
 * @param action :WRITE, READ. Option NONE will give error
 * @return FileLink
 */
FileLink LockFile( FileLocks *file_locks, const char *file_name, Action action );

/**
 * @brief unlocks the file given the file link
 * @param file_locks :lock of the files
 * @param file_link_ptr :pointer of pointer to file_link : is set to null when ran
 * @return none 
 */
void UnlockFile( FileLocks *file_locks, FileLink *file_link_ptr );
#endif