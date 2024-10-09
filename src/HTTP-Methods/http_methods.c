/**
* Created by Fabert Charles
*
*/
/*Included Libraries*/
#include <HTTP-Methods/http_methods.h>
#include <Request-Parser/request_parser.h>

// For opening file
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>

// For closing file
#include <unistd.h>

// For error handling
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>

#define MAX_PUT_MESSAGE_BUFF 10000

/*Type Definitions*/
// struct for holding a single uri
typedef struct LockFile {
    int numThreads; // Number of threads that are present here
    char URI[100];
    int numReaders;
    sem_t numReadKey;
    sem_t fileWrite; // semaphore for file write
}LockFile;

// Struct for Holding multiple file locks
typedef struct URILock {
    int size;
    sem_t changeList;
    LockFile* files;
}URILock;

/*Extern Vars*/
extern URILock* e_uriLocks; // pointer to the uri locks

/*Private Function Declarations*/
/** @brief Locks File
*   @param URI: File to lock
*   @param method: http method that need to lock file
*   @param indexFile: index of file if found with lock already
*   @return 0 for thread to go,
            1 for thread to wait for head or get to finish from other thread,
            2 for thread to wait for put command to finish from some thread
*/
int URILockFunc(char* URI, Methods method, int* indexFile);

/** @brief Releases the uri lock the file
*   @param index:  index of file to be release
*   @param method: Method of http request
*   @return void
*/
void URIReleaseFunc(int index, Methods method);

/** @brief prints to the log file of the server
*   @param reqNum: The number given to client request
*   @param logFD: The file descriptor of the server log file
*   @param file: The file name the request wanted to use
*   @param method:  The method of the client request
*   @return void
*/
void LogFilePrint(long int reqNum, int logFD, int statusCode, char *file, char* method);

/*Function defintions*/
int http_methods_PutReq(char* file, char* buffer, int clientFD, long int* currentPosBuf,
    long int* bytesRead, long int contentLength, long int reqNum, int logFD) {

    // creating a temp file
    char tempFile[] = "TEMPFILE-XXXXXX";
    int fd = mkstemp(tempFile);

    // checking content_length
    long int bytesLeftToWrite = contentLength - (*bytesRead - *currentPosBuf);


	// Variables used for interrupts
    int bytesWritten = 0; // used to check if write actually writes

	long int messageLen = 0;
	char messageHolder[MAX_PUT_MESSAGE_BUFF];


    // Copy buffer into messageHolder
    for (int i = 0; i < *bytesRead; ++i) {
        messageHolder[i] = buffer[i + *currentPosBuf];
    }


	messageLen = *bytesRead - *currentPosBuf;

	if (bytesLeftToWrite == 0) {

		bytesWritten = write(fd, messageHolder, messageLen);
	}
	else {
		while (bytesLeftToWrite != 0 && !ev_signQuit) {
			if (messageLen == MAX_PUT_MESSAGE_BUFF) {
				bytesWritten = write(fd, messageHolder, messageLen);
				if (bytesWritten < 0) break;
				messageLen = 0;
			}
			else {
				*bytesRead = read(clientFD, messageHolder + messageLen, MAX_PUT_MESSAGE_BUFF - messageLen);
				if (*bytesRead <= 0) break;
				bytesLeftToWrite -= *bytesRead;
				messageLen += *bytesRead;
			}
		}
		if (bytesLeftToWrite >= 0 && *bytesRead >= 0 && !ev_signQuit) {
			bytesWritten = write(fd, messageHolder, messageLen);
		}
		else {
			http_methods_StatusPrint(clientFD, ISE_);
			LogFilePrint(reqNum, logFD, 500, file, "PUT"); // Printing to Log
		}
	}

    // Lock the File
    int fileIndex;
    close(fd); // closing temp file

    int lockResult = URILockFunc(file, PUT, &fileIndex); // locking file

    if (lockResult == 1)
        while(sem_wait(&(e_uriLocks->files[fileIndex].fileWrite)) < 0); // getting write lock

    // need to check if file exists
    bool fileExist = false;

    if (access(file, F_OK) == 0)
        fileExist = true;
    else
        fileExist = false;

    rename(tempFile, file);

    /*If I cannot process the full request because of some interruption*/
    if (bytesLeftToWrite != 0) {
        // need to clear file
        fd = open(file, O_TRUNC | O_WRONLY);
        close(fd);
    }

    // success and Request Printing
    if (!fileExist) {
        http_methods_StatusPrint(clientFD, CREATED_);
        LogFilePrint(reqNum, logFD, 201, file, "PUT"); // Printing to Log
        URIReleaseFunc(fileIndex, PUT);
    }
    else {
        http_methods_StatusPrint(clientFD, OK_);
        LogFilePrint(reqNum, logFD, 200, file, "PUT"); // Printing to Log
        URIReleaseFunc(fileIndex, PUT);
    }

    return 0;
}


int http_methods_GetReq(char* file, int clientFD, long int reqNum, int logFD) {
    // first checking if file exists
    int fd; // file descriptor for file wanted
    struct stat fileStat; // Holds status of file
    int bytesWritten = 0; // used to check if write actually writes
    unsigned long bytesContinuation = 0; // If only a partial amount of bytes written

    // Acquire URI Lock for file
    int URILockIndex = 0;
    int statusURILock = URILockFunc(file, GET, &URILockIndex); // acquring URI lock

    // Write. So only waiting for write
    if (statusURILock == 2) {
        while (sem_wait(&(e_uriLocks->files[URILockIndex].numReadKey)) < 0); // getting lock to change counter

        ++(e_uriLocks->files[URILockIndex].numReaders); // increment num of readers

        // Getting lock to stop writing to file
        if (e_uriLocks->files[URILockIndex].numReaders == 1) {
            while (sem_wait(&(e_uriLocks->files[URILockIndex].fileWrite)) < 0); // getting write lock
        }

        sem_post(&(e_uriLocks->files[URILockIndex].numReadKey)); // release numReader Key
    }
    // end of lock aquiring
    // if file is null
    if (strlen(file) == 0) {
        http_methods_StatusPrint(clientFD, ISE_);

        /*Printing to log*/
        LogFilePrint(reqNum, logFD, 500, file, "GET"); // Printing to Log
        URIReleaseFunc(URILockIndex, GET);
        return -1;
    }

    /* Checking for file existence*/
    bool fileExist = false;
    if (access(file, F_OK) == 0)
        fileExist = true;
    else
        fileExist = false;

    if (fileExist) {
        fd = open(file, O_RDONLY); // read the file

        // File error
        if (fd < 0) {
            if (errno == EACCES)
                http_methods_StatusPrint(clientFD, ISE_);
            else
                http_methods_StatusPrint(clientFD, ISE_);

            /*Printing to log*/
            LogFilePrint(reqNum, logFD, 500, file, "GET"); // Printing to Log
            URIReleaseFunc(URILockIndex, GET);
            return -1;
        }

        fstat(fd, &fileStat);

        // checking for directory
        if (S_ISDIR(fileStat.st_mode) != 0) {
            close(fd);
            http_methods_StatusPrint(clientFD, ISE_);

            /*Printing to log*/
            LogFilePrint(reqNum, logFD, 500, file, "GET"); // Printing to Log
            URIReleaseFunc(URILockIndex, GET);
            return -1;
        }
    }
    // if the file doesn't exist
    else {
        http_methods_StatusPrint(clientFD, NOT_FOUND_);

        /*Printing to log*/
        LogFilePrint(reqNum, logFD, 404, file, "GET"); // Printing to Log
        URIReleaseFunc(URILockIndex, GET);
        return -1;
    }

    // read the data from file
    char buffer[4096]; // create buffer for reading data
    char serverMess[1000];
    long int bytesRead = 0;
    sprintf(serverMess, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", fileStat.st_size);

    // Checks if intterrupted and writes
    do {
        bytesWritten = write(clientFD, serverMess, strlen(serverMess));

        // CHECKING IF ERROR AND NOT INTERRUPTED
        if ((bytesWritten < 0) && (errno != EINTR)) {
            close(fd); // close the file
            http_methods_StatusPrint(clientFD, ISE_);
            LogFilePrint(reqNum, logFD, 500, file, "GET"); // Printing to Log
            URIReleaseFunc(URILockIndex, GET);
            return -1;
        }

        // Increment bytes
        if (bytesWritten > 0) {
                bytesContinuation += bytesWritten;
        }
    } while (((bytesWritten < 0) && (errno == EINTR)) || ((bytesContinuation < strlen(serverMess)) && (bytesContinuation > 0)));

    bytesContinuation = 0; // reset how many bytes written

    /*writing file content to client FD*/
    while(1) {
        bytesRead = read(fd, buffer, 4096); // read bytes

        if (bytesRead == 0) { // If there's nothing to read
            break;
        }

        if (bytesRead < 0) {
            // continue
            if (errno == EINTR) continue;
            else {
                close(fd); // close the file

                /*Printing to log*/
                http_methods_StatusPrint(clientFD, ISE_);
                LogFilePrint(reqNum, logFD, 500, file, "GET"); // Printing to Log
                URIReleaseFunc(URILockIndex, GET);
                return -1;
            }
        }

        // Now writing to client
        do {
            bytesWritten = write(clientFD, buffer, bytesRead);
            if ((bytesWritten < 0) && (errno != EINTR)) {
                close(fd); // close the file
                http_methods_StatusPrint(clientFD, ISE_);
                LogFilePrint(reqNum, logFD, 500, file, "GET"); // Printing to Log
                URIReleaseFunc(URILockIndex, GET);
                return -1;
            }
            // Increment bytes
            if (bytesWritten > 0) {
                bytesContinuation += bytesWritten;
            }
        } while (((bytesWritten < 0) && (errno == EINTR)) || ((bytesContinuation < (unsigned long)bytesRead) && (bytesContinuation > 0)));
        bytesContinuation = 0; // resetting the bytesContinuation
    }

    close(fd);

    /*Printing to log*/
    LogFilePrint(reqNum, logFD, 200, file, "GET"); // Printing to Log
    URIReleaseFunc(URILockIndex, GET);
    return 0;
}

int http_methods_HeadReq(char* file, int clientFD, long int reqNum, int logFD) {
    // first checking if file exists
    int fd; // Holds file descriptor of file that client wants
    struct stat fileStat; // Status of opening file
    int bytesWritten = 0;
    int bytesContinuation = 0;

    // Acquiring URI lock
    int URILockIndex = 0;
    int statusURILock = URILockFunc(file, HEAD, &URILockIndex); // acquring URI lock
    if (statusURILock == 2) { // Write. So only waiting for write
        while (sem_wait(&(e_uriLocks->files[URILockIndex].numReadKey)) < 0); // getting lock to change counter

        ++(e_uriLocks->files[URILockIndex].numReaders); // increment num of readers
        if (e_uriLocks->files[URILockIndex].numReaders == 1) { // If the first
            while (sem_wait(&(e_uriLocks->files[URILockIndex].fileWrite)) < 0); // getting write lock
        }
        sem_post(&(e_uriLocks->files[URILockIndex].numReadKey)); // release numReader Key
    }

    // if file is null
    if (strlen(file) == 0) {
        http_methods_StatusPrint(clientFD, ISE_);
        /*Printing to log*/
        LogFilePrint(reqNum, logFD, 500, file, "HEAD"); // Printing to Log
        URIReleaseFunc(URILockIndex, HEAD);
        return -1;
    }

    /* Checking for file existence*/
    bool fileExist = false;
    if (access(file, F_OK) == 0)
        fileExist = true;
    else
        fileExist = false;

    if (fileExist) {
        fd = open(file, O_RDONLY); // read the file
        if (fd < 0) // File error
        {
            if (errno == EACCES)
                http_methods_StatusPrint(clientFD, ISE_);
            else
                http_methods_StatusPrint(clientFD, ISE_);

            /*Printing to log*/
            LogFilePrint(reqNum, logFD, 500, file, "HEAD"); // Printing to Log
            URIReleaseFunc(URILockIndex, HEAD);
            return -1;
        }
        fstat(fd, &fileStat);

        // checking for directory
        if (S_ISDIR(fileStat.st_mode) != 0) {
            close(fd);
            http_methods_StatusPrint(clientFD, ISE_);
            LogFilePrint(reqNum, logFD, 500, file, "HEAD"); // Printing to Log
            URIReleaseFunc(URILockIndex, HEAD);
            return -1;
        }
    }
    // if the file doesn't exist
    else {
        http_methods_StatusPrint(clientFD, NOT_FOUND_);
        LogFilePrint(reqNum, logFD, 404, file, "HEAD"); // Printing to Log
        URIReleaseFunc(URILockIndex, HEAD);
        return -1;
    }

    // read the data from file
    char serverMess[1000];
    sprintf(serverMess, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", fileStat.st_size);

    // Checks if intterrupted and writes
    do {
        bytesWritten = write(clientFD, serverMess, strlen(serverMess));
        // CHECKING IF ERROR AND NOT INTERRUPTED
        if ((bytesWritten < 0) && (errno != EINTR)) {
            close(fd); // close the file
            http_methods_StatusPrint(clientFD, ISE_);
            LogFilePrint(reqNum, logFD, 500, file, "HEAD"); // Printing to Log
            URIReleaseFunc(URILockIndex, HEAD);
            return -1;
        }
        // Increment bytes
        if (bytesWritten > 0) {
            bytesContinuation += bytesWritten;
        }
    } while (((bytesWritten < 0) && (errno == EINTR)) || ((bytesContinuation < (int)strlen(serverMess)) && (bytesContinuation > 0)));

    LogFilePrint(reqNum, logFD, 200, file, "HEAD"); // Printing to Log
    close(fd);
    URIReleaseFunc(URILockIndex, HEAD);
    return 0;
}

void LogFilePrint(long int reqNum, int logFD, int statusCode, char *file, char* method) {
    // lock file
    while (flock(logFD, LOCK_EX) < 0); // getting flock

    char log_message[1000]; // Creates log message

    /*If the methods is PUT*/
    sprintf(log_message, "%s,/%s,%d,%ld\n", method, file, statusCode, reqNum);
    int bytesWritten = 0; // holds how many bytes written
    int bytesContinuation = 0;

    // If write is interrupted, then try again
    do {
       bytesWritten = write(logFD, log_message, strlen(log_message)); // write to log

       // Increment bytes
       if ((bytesWritten < 0) && (errno != EINTR)) break;

       if (bytesWritten > 0) bytesContinuation += bytesWritten;
    } while (((bytesWritten < 0) && (errno == EINTR)) || ((bytesContinuation < (int)strlen(log_message)) && (bytesContinuation > 0)));

    // release lock on log
    flock(logFD, LOCK_UN);
}


void http_methods_StatusPrint(int clientFD, StatusC status) {
    int bytesWritten = 0; // Bytes written by write()
    int bytesContinuation = 0; // Continuing bytes for write()
    switch (status) {
    case OK_:
        do {
           bytesWritten = write(clientFD, "HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nOK\r\n", 42);
           // Increment bytes
           if (bytesWritten > 0) {
               bytesContinuation += bytesWritten;
           }
           // adding If not errno == INTR
           if ((bytesWritten < 0) && (errno != EINTR)) {
               break;
           }

        } while ( ((bytesWritten < 0) && (errno == EINTR)) || ((bytesContinuation < 42) && (bytesContinuation > 0)) );
        break;

    case CREATED_:
        do {
            bytesWritten = write(clientFD, "HTTP/1.1 201 Created\r\nContent-Length: 9\r\n\r\nCreated\r\n", 52);
            // Increment bytes
            if (bytesWritten > 0) {
                bytesContinuation += bytesWritten;
            }
            // adding If not errno == INTR
            if ((bytesWritten < 0) && (errno != EINTR)) {
                break;
            }
        } while (((bytesWritten < 0) && (errno == EINTR)) || ((bytesContinuation < 52) && (bytesContinuation > 0)));
        break;

    case BAD_REQUEST_:
        do {
            bytesWritten = write(clientFD, "HTTP/1.1 400 Bad Request\r\nContent-Length: 13\r\n\r\nBad Request\r\n", 61);
            // Increment bytes
            if (bytesWritten > 0) {
                bytesContinuation += bytesWritten;
            }
            // adding If not errno == INTR
            if ((bytesWritten < 0) && (errno != EINTR)) {
                break;
            }
        } while (((bytesWritten < 0) && (errno == EINTR)) || ((bytesContinuation < 61) && (bytesContinuation > 0)));
        break;

    case FORBIDDEN_:
        do {
           bytesWritten = write(clientFD, "HTTP/1.1 403 Forbidden\r\nContent-Length: 11\r\n\r\nForbidden\r\n", 57);
           // Increment bytes
           if (bytesWritten > 0) {
               bytesContinuation += bytesWritten;
           }
           // adding If not errno == INTR
           if ((bytesWritten < 0) && (errno != EINTR)) {
               break;
           }
        } while (((bytesWritten < 0) && (errno == EINTR)) || ((bytesContinuation < 57) && (bytesContinuation > 0)));
        break;

    case NOT_IMP_:
        do {
            bytesWritten = write(clientFD, "HTTP/1.1 501 Not Implemented\r\nContent-Length: 17\r\n\r\nNot Implemented\r\n", 69);
            // Increment bytes
            if (bytesWritten > 0) {
                bytesContinuation += bytesWritten;
            }
            // adding If not errno == INTR
            if ((bytesWritten < 0) && (errno != EINTR)) {
                break;
            }
        } while (((bytesWritten < 0) && (errno == EINTR)) || ((bytesContinuation < 69) && (bytesContinuation > 0)));
        break;

    case NOT_FOUND_:
        do {
            bytesWritten = write(clientFD, "HTTP/1.1 404 Not Found\r\nContent-Length: 11\r\n\r\nNot Found\r\n", 57);
            // Increment bytes
            if (bytesWritten > 0) {
                bytesContinuation += bytesWritten;
            }
            // adding If not errno == INTR
            if ((bytesWritten < 0) && (errno != EINTR)) {
                break;
            }
        } while (((bytesWritten < 0) && (errno == EINTR)) || ((bytesContinuation < 57) && (bytesContinuation > 0)));
        break;

    case ISE_:
        do {
            bytesWritten = write(clientFD, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 23\r\n\r\nInternal Server Error\r\n", 81);
            // Increment bytes
            if (bytesWritten > 0) {
                bytesContinuation += bytesWritten;
            }
            // adding If not errno == INTR
            if ((bytesWritten < 0) && (errno != EINTR)) {
                break;
            }
        } while (((bytesWritten < 0) && (errno == EINTR)) || ((bytesContinuation < 81) && (bytesContinuation > 0)));
        break;
    }
}

int URILockFunc(char *URI, Methods method, int *indexFile) {
    // need to lock semaphor
    int indexes_with_no_file[e_uriLocks->size];
    int index_stop_no_files = 0;

    // &&Get lock for changing list
    while (sem_wait(&(e_uriLocks->changeList)) < 0);

    /*Looking to see which spot is free to change*/
    int SizeList = e_uriLocks->size;

    for (int i = 0; i < SizeList; ++i) {
        // checking for similar uris
        LockFile* currentFile = &(e_uriLocks->files[i]); // holding file struct

        if (strcmp(currentFile->URI, URI) != 0) { // found a different uri
            if (currentFile->numThreads == 0) { // if no threads using this file
                indexes_with_no_file[index_stop_no_files] = i; // storing index of free file
                ++index_stop_no_files; // increment index
            }
            continue; // go to next file

        }
        /*If a similar file is found*/
        else {
            *indexFile = i; // getting index

            if (currentFile->numThreads == 0) { // if no threads working on this file
                currentFile->numThreads += 1; // Increment thread count using file

                /*Checkng method*/
                if (method != PUT) {
                    currentFile->numReaders += 1; // increase number of readers
                }
                while(sem_wait(&(currentFile->fileWrite)) < 0); // get the write semaphore

                sem_post(&(e_uriLocks->changeList)); // release list change semphore
                return 0; // Mean that first semaphore is gotten
            }
            // If there is already working threads present
            else {
                currentFile->numThreads += 1;  // Increment thread count using file
                if (method == PUT) {
                    sem_post(&(e_uriLocks->changeList)); // release list change semphore
                    return 1; // returns 1 to signify thread to wait for PUT
                }
                else {
                    sem_post(&(e_uriLocks->changeList)); // release list change semphore
                    return 2; // returns 2 to signify thread to wait for GET or HEAD
                }
            }
        }
    }
    // If no thread found during that looking
    LockFile* currentFile = &(e_uriLocks->files[indexes_with_no_file[0]]); // geting first file not used
    *indexFile = indexes_with_no_file[0]; // getting the index
    currentFile->numThreads += 1; // setting first thread usage

    // copying string
    for (unsigned int i = 0; i < strlen(URI); ++i) {
        currentFile->URI[i] = URI[i];
    }

    currentFile->URI[strlen(URI)] = '\0'; // adding null char
    while (sem_wait(&(currentFile->fileWrite)) < 0); // getting write semaphore

    // release lock
    if (method != PUT) {
        currentFile->numReaders += 1;
    }
    sem_post(&(e_uriLocks->changeList)); // release list change semphore
    return 0; // Means first semaphore gotten

}


void URIReleaseFunc(int index, Methods Meth) {
    if (Meth != PUT) { // If the Method was a HEAD or GET
        while (sem_wait(&(e_uriLocks->files[index].numReadKey)) < 0); // Getting lock to change number waitng on thread
        e_uriLocks->files[index].numReaders -= 1;

        // checking if zero
        if (e_uriLocks->files[index].numReaders == 0) {
            sem_post(&(e_uriLocks->files[index].fileWrite)); // release the write lock
        }

        sem_post(&(e_uriLocks->files[index].numReadKey));

    }
    else {
        sem_post(&(e_uriLocks->files[index].fileWrite)); // release the write lock

    }
    while (sem_wait(&(e_uriLocks->changeList)) < 0); // &&Get lock for changing list
    e_uriLocks->files[index].numThreads -= 1; // remove 1 number threads waiting
    sem_post(&(e_uriLocks->changeList)); // release list change semphore
}
