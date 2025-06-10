/**
* Created By Fabert Charles
*
* HTTP SERVER File
*/

/*Included Libraries*/
// For opening file
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// For closing file
#include <unistd.h>

// For error handling
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <signal.h>

// For Threads and atomic
#include <stdatomic.h>
#include <pthread.h>
#include <semaphore.h>

// Included Custom Libraries
#include "Bind/bind.h"
#include "HTTP-Methods/http_methods.h"
#include "HTTP-Server/httpserver.h"
#include "Queue/queue.h"
#include "Request-Parser/request_parser.h"


/*Global Vars*/
URILock *e_uriLocks; // pointer to the uri locks

signalThreads *g_signalThreads; // pointer to signal threads

/*Type Definitions*/
#define REQUEST_BUFFER_SIZE 2048
#define CLIENT_BUFFER_SIZE 1024

// Struct for holding a single uri
typedef struct LockFile {
    int numThreads; // Number of threads that are present here
    char URI[100];
    int numReaders; // How many threads are reading the file
    sem_t numReadKey; // Key for readers to read file
    sem_t fileWrite; // semaphore for file write
} LockFile;

// Struct to hold the list of uris
typedef struct URILock {
    int size;
    sem_t changeList;
    LockFile *files;
} URILock;

// Struct to signal threads to finissh
typedef struct signalThreads {
    int size;
    atomic_bool *finishThreads; // Holds an array for finished threads
} signalThreads;


// Enum to show the current state of the request
// A consecutive /r/n/r/n will signify the end of a request


//Flag for interrupt
volatile atomic_bool ev_interrupt_received
    = false; // If terminate signal happens, quit. Also, atomic variable

// Queue for holding client_fd
queue_t *g_queueRequest;

// Holds logfile FD
int logFileFD = STDERR_FILENO; // default logFile Descriptor

/*Function Declarations*/

/** @brief Initializes URILocks
*   @param numThread:  The number of thread available
*   @return void
*/
void URILockInit(int numThreads);

/** @brief cleans up URILock initialized in URILockInit
*   @return Void
*/
void FreeUriLocks();

/**  @brief Initializes array which holds if thread is finished and ended
*    @param numThread:  How many thread available to process requests
*    @return void
*/
void FinishThreadInit(int numThreads);

/** @brief Cleans up array initialized in FinishedThreadInit
*   @return void
*/
void FinishThreadInitFree();

/**  @brief Function in which threads will process requests from clients
*    @param arg:
*/
void *Worker_Request(void *arg);

/** @brief Checks if the optional arguments are valid
*   @param l_flag:  True if l optional argument available
*   @param t_flag:  True if t optional argument available
*   @param argc: User input count
*   @param argv:  User input values
*   @param numThreads: Number of threads
*   @param log_file:  Where the server log is held
*   @return void
*/
void OptionalArgumentChecker(
    bool *l_flag, bool *t_flag, int argc, char *argv[], int *numThreads, char *log_file);

//Signal Interrupt
/**  @brief Handles signal interrupt
*    @param signum:  signal number
*    @return void
*/
void sig_Interrupt(int signum) {
    printf("Start Signal Handling\n");
    if (signum == SIGTERM) {
        ev_interrupt_received = true; // set sign quit to true
    }
}

int main(int argc, char *argv[]) {
    // Setting up interrupt struct
    struct sigaction interrup_sign;
    memset(&interrup_sign, 0, sizeof(interrup_sign));
    interrup_sign.sa_handler = sig_Interrupt;
    sigaction(SIGTERM, &interrup_sign, NULL);

    // Checking for user arguments
    char log_file[1000]; // Holds log file file

    //Flags for optional arguments
    bool l_flag = false;
    bool t_flag = false;
    int numThreads = 4; // default # of threads

    // If there are no arguments
    if (argc < 2) {
        warnx("Too few arguments:\nusage: ./httpserver [-t threads] [-l logfile] <port>");
        exit(1);
    }

    // Checking optional arguments
    OptionalArgumentChecker(&l_flag, &t_flag, argc, argv, &numThreads, log_file);

    // Getting Port Number
    // First Checking there is an actual number available
    if (optind == argc) {
        warnx("No port given: ./httpserver [-t threads] [-l logfile] <port>");
        exit(1);
    }

    uint16_t portNum = portCheck(argv[optind]); // Checking for valid port number
    int socketFD = create_listen_socket(portNum);
    // Exit if not valid
    if (socketFD < 0) {
        warnx("bind: %s", strerror(errno));
        exit(1);
    }

    // Initializing queue for requests
    g_queueRequest = queue_new(numThreads); // creates a queue size of threads

    // Opening log file if found
    if (l_flag) {
        // If the File Exists
        if (access(log_file, F_OK) == 0) { // Checking if file exists
            logFileFD = open(log_file, O_TRUNC | O_WRONLY); // getting logFileFD

            // File error
            if (logFileFD < 0) {
                warnx("logfile opener: %s", strerror(errno));
                exit(1);
            }
        }
        // If the file doesn't exitst
        else {
            logFileFD = open(log_file, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

            // File error
            if (logFileFD < 0) {
                warnx("logfile opener: %s", strerror(errno));
                exit(1);
            }
        }
    }

    URILockInit(numThreads);

    FinishThreadInit(numThreads);

    pthread_t worker_threads[numThreads]; // create numThreads workers

    int threadNumber[numThreads];

    for ( int i = 0; i < numThreads; ++i ) {
        threadNumber[i] = i;
    }

    for ( int i = 0; i < numThreads; ++i ) {
        void *ThreadArg = &(threadNumber[i]); // Giving each thread an identifier
        pthread_create(worker_threads + i, NULL, Worker_Request, ThreadArg);
    }

    // Processing Client
    printf("Starting Server\n");

    while ( !ev_interrupt_received ) {
        int *client_fd = malloc( sizeof(int) );
        *( client_fd ) = accept( socketFD, NULL, NULL ); // waiting for connections

        /*If accept produces an error*/
        if ( ( *client_fd ) < 0 ) {
            free( client_fd );

            if ( errno == EINTR ) { // need to break free and end program
                break;
            }
            else {
                continue; // try again
            }
        }

        // if client_fd not put in queue because sigterm
        if ( !queue_push( g_queueRequest, client_fd ) ) {
            free( clientF D);
        }
    }

    bool allThreadFinish = false;

    // Waits for all the threads to finish processing last requests
    printf("Handling closing threads here\n");

    // Sending to command to queue to wake up threads
    queue_condition_pop(g_queueRequest);
    queue_condition_push(g_queueRequest);
    while (!allThreadFinish) {
        int numFin = 0; // Shows how many threads finished
        for (int i = 0; i < g_signalThreads->size; ++i) {
            if (g_signalThreads->finishThreads[i]) {
                ++numFin; // Increment counter for finish
            }
        }

        // Checking numFin
        if (numFin == g_signalThreads->size)
            allThreadFinish = true; // If all threads finish set to notify
        else
            usleep(100); // sleep for 100 microseconds
    }

    // Joining threads
    for (int i = 0; i < numThreads; ++i) {
        pthread_join(worker_threads[i], NULL);
    }

    printf("End of Handling Requests\n");

    // Removing queue structs present
    while (queue_size(g_queueRequest) != 0) {
        void *strhere;
        queue_pop(g_queueRequest, &strhere);
        free(strhere);
    }

    queue_delete(&g_queueRequest);
    FreeUriLocks();
    FinishThreadInitFree();
    fsync(logFileFD); //** Freeing memory for URI lock
    close(logFileFD); // Close log file
    return 0;
}

uint16_t portCheck(char *port) {
    // Checking values of char
    for (unsigned int i = 0; port[i] != '\0'; ++i) {
        if (port[i] < '0' || port[i] > '9') {
            warnx("invalid port number: %s", port);
            exit(1);
        }
    }

    // Turning string to long
    uint16_t result = strtol(port, NULL, 10);
    return result;
}


void AppendingClientBufferToRequest( Request *request, Buffer *client_buffer ) {
    while ( client_buffer->current_index < client_buffer->length ) {

        char c = client_buffer.data[ client_buffer->current_index ];
        ++( client_buffer->current_index );

        request->buffer.data[ request->buffer.current_index ] = c;
        ++( request->buffer.current_index );
        ++( request->buffer.length );

        switch ( request->current_state ) {
            case INITIAL_STATE:
                if ( c == '\r' ) {
                    request->current_state = FIRST_R;
                }
            break;

            case FIRST_R:
                if ( c == '\n' ) {
                    request->current_state = FIRST_N;
                }
            break;

            case FIRST_N:
                if (c == '\r' ) {
                    request->current_state = SECOND_R;
                }
            break;

            case SECOND_R:
                if ( c == '\n' ) {
                    request->current_state = REQUEST_COMPLETE;
                }
            break;

            default;
        }

    }
}

void *WorkerRequest( void *arg ) {
    int threadNum = *( ( int * ) arg) ;

    Buffer client_buffer = {
        .data = malloc( sizeof( char ) * CLIENT_BUFFER_SIZE ),
        .max_size = CLIENT_BUFFER_SIZE,
        .length = 0,
        .current_index = 0
    };

    Request request = {
        .buffer = {
            .data = malloc( sizeof ( char ) * REQUEST_BUFFER_SIZE ),
            .max_size = REQUEST_BUFFER_SIZE,
            .length = 0,
            .current_index = 0
        },

        .current_state = INITIAL_STATE
        .type = NOT_VALID,

        .headers = {
            .content_length = 0,
            .request_id = 0
        }

        .file = malloc( sizeof ( char ) * FILE_NAME_LENGTH )
    };

    bool client_closed = false;

    // get client descriptor from queue
    void *client_temp = NULL; // only used to get client fd num and 

    int client_fd;

    while ( !ev_interrupt_received ) {
        // reseting variables
        client_closed = false;

        request.buffer.length = 0;
        request.current_index = 0;

        request.current_state = INITIAL_STATE;
        request.type = NOT_VALID;
        request.headers.content_length = -1;
        request.headers.request_id = -1;
        request.headers.expect = false;

        if ( !QueuePop( g_queueRequest, &client_temp ) ) {
            break;
        }

        client_fd = *( ( int * ) client_temp );
        free( client_temp );

        // need to set client to nonblocking
        fcntl( client_fd, F_SETFL, O_NONBLOCK );

        while ( !ev_interrupt_received && ( request.current_state != REQUEST_COMPLETE ) ) {
            int client_bytes_read = read( client_fd, client_buffer.data, client_buffer.max_size );

            if ( client_bytes_read < 0 ) {
                continue;
            }
            else if ( client_bytes_read == 0 ) {
                client_closed = true;
                break;
            }
            else {
                client_buffer.length = client_bytes_read;
                client_buffer.current_index = 0;

                AppendingClientBufferToRequest( &request, &client_buffer );
            }
        }

        if ( ev_interrupt_received ) {
            close( client_fd );
            break;
        }

        /*Checking format of request */
        if ( !client_closed ) {

            // Checking Request
            if ( RequestChecker( &request ) != 0 ) {
                http_methods_StatusPrint( client_fd, BAD_REQUEST_ );
                close( client_fd );
                continue;
            }

            // Checking headerfields
            if ( HeaderFieldChecker( &request ) != 0 ) {
                http_methods_StatusPrint( client_fd, BAD_REQUEST_ );
                close( client_fd );
                continue;
            }


            // check if no valid Meth
            if (Meth == NO_VALID) {
                // Throw proper error
                http_methods_StatusPrint(client_fd, NOT_IMP_);
                close(client_fd);
                continue;
            }

            // Checking for file existence
            int resultMeth = 0;

            // PUT
            if (Meth == PUT) {
                resultMeth = http_methods_PutReq(fileName, client_buffer, client_fd, &currentPosBuff,
                    &client_bytes_read
, Content_length, Request_ID, logFileFD);

            }
            // GET
            else if (Meth == GET)
                resultMeth = http_methods_GetReq(fileName, client_fd, Request_ID, logFileFD);
            // HEAD
            else
                resultMeth = http_methods_HeadReq(fileName, client_fd, Request_ID, logFileFD);
        }
        /*Resetting all variables to original values*/
        close(client_fd);
    }

    g_signalThreads->finishThreads[threadNum] = true;
    return NULL;
}

/*URI locks that helps with the reader and write problem*/
void URILockInit(int numThreads) {
    e_uriLocks = malloc(sizeof(URILock)); // creating array of locks for number of threads
    e_uriLocks->size = numThreads;
    sem_init(&(e_uriLocks->changeList), 1, 1); // Initializing lock to check array
    e_uriLocks->files = malloc(numThreads * sizeof(LockFile));
    for (int i = 0; i < numThreads; ++i) {
        (e_uriLocks->files)[i].URI[0] = '\0';
        (e_uriLocks->files)[i].numReaders = 0;
        (e_uriLocks->files)[i].numThreads = 0; // settign waiting/working threads to zero.
        sem_init(&((e_uriLocks->files)[i].fileWrite), 1, 1);
        sem_init(&((e_uriLocks->files)[i].numReadKey), 1, 1);
    }
}

void FreeUriLocks() {
    int size = e_uriLocks->size;
    for (int i = 0; i < size; ++i) {
        // unitialize all semaphores used
        sem_destroy(&((e_uriLocks->files)[i].fileWrite));
        sem_destroy(&((e_uriLocks->files)[i].numReadKey));
    }
    // now free the memory
    free(e_uriLocks->files);
    sem_destroy(&(e_uriLocks->changeList));
    free(e_uriLocks);
    e_uriLocks = NULL;
}

void FinishThreadInit(int numThreads) {
    g_signalThreads = malloc(sizeof(signalThreads)); // malloc array for singal threads
    g_signalThreads->size = numThreads; // Holds How many threads present
    g_signalThreads->finishThreads
        = calloc(numThreads, sizeof(atomic_bool)); // Initializing boolean array
}

void FinishThreadInitFree() {
    free(g_signalThreads->finishThreads); // free bool array
    free(g_signalThreads);
    g_signalThreads = NULL;
}

void OptionalArgumentChecker(
    bool *l_flag, bool *t_flag, int argc, char *argv[], int *numThreads, char *log_file) {
    int char_get = 0; // Holds char value
    while ((char_get = getopt(argc, argv, ":t:l:")) != -1) {
        switch (char_get) {
            /*Checking for number of threads*/
        case 'l':
            *l_flag = true;
            // If not string after
            if (optarg != NULL) {
                strcpy(log_file, optarg); // copies string into log_file
            } else {
                warnx(
                    "invalid option input:\nusage: ./httpserver [-t threads] [-l logfile] <port>");
                exit(1);
            }
            break;

            /*Checking for logfile*/
        case 't':
            *t_flag = true;
            if (optarg != NULL) {
                // checking if valid numbers:
                for (uint32_t i = 0; i < strlen(optarg); ++i) {
                    if ((optarg[i] < '0') || (optarg[i] > '9')) {
                        // need to exit program
                        warnx(
                            "invalid option input: ./httpserver [-t threads] [-l logfile] <port>");
                        exit(1);
                    }
                }
                *numThreads = atoi(optarg);
            } else {
                warnx("invalid option input: ./httpserver [-t threads] [-l logfile] <port>");
                exit(1);
            }
            break;

            /*If some random option*/
        case '?': warnx("invalid option: ./httpserver [-t threads] [-l logfile] <port>"); exit(1);

        case ':':
            warnx("No value given for option %c: ./httpserver [-t threads] [-l logfile] <port>",
                optopt);
            exit(1);
        }
    }
}
