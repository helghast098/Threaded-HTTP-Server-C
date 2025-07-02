/**
* Created By Fabert Charles
*
* HTTP SERVER File
*/


// Included Custom Libraries
#include "bind/bind.h"
#include "file_lock/file_lock.h"
#include "http_methods/http_methods.h"
#include "server/httpserver.h"
#include "queue/queue.h"
#include "request_parser/request_parser.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <signal.h>
#include <stdatomic.h>
#include <pthread.h>
#include <semaphore.h>

SignalThreads *g_SignalThreads; // pointer to signal threads

typedef struct SignalThreads {
    int size;
    atomic_bool *finishThreads; // Holds an array for finished threads
} SignalThreads;

volatile atomic_bool ev_interrupt_received = false;

int log_file_fd = STDERR_FILENO; // default logFile Descriptor

void FinishedThreadsInit( int number_of_thr eads);

void FinishedThreadsInitFree();

void *WorkerRequest( void *arg );

void OptionalArgumentChecker(
    bool *l_flag, bool *t_flag, int argc, char *argv[], int *number_of_threads, char *log_file);

void SignalInterrupt(int signum) {
    if (signum == SIGTERM) {
        ev_interrupt_received = true;
    }
}


int main(int argc, char *argv[]) {
    char log_file[ 1000 ]; // Holds log file

    // Setting up interrupt struct
    struct sigaction interrup_sign;
    memset(&interrup_sign, 0, sizeof(interrup_sign));
    interrup_sign.sa_handler = SignalInterrupt;
    sigaction(SIGTERM, &interrup_sign, NULL);

    //Flags for optional arguments
    bool l_flag = false;
    bool t_flag = false;
    int number_of_threads = 4;

    // If there are no arguments
    if (argc < 2) {
        warnx("Too few arguments:\nusage: ./httpserver [-t threads] [-l logfile] <port>");
        exit(1);
    }

    // Checking optional arguments
    OptionalArgumentChecker( &l_flag, &t_flag, argc, argv, &number_of_threads, log_file );

    if ( l_flag ) {
        if ( access( log_file, F_OK ) == 0 ) {
            log_file_fd = open(log_file, O_TRUNC | O_WRONLY);

            if (log_file_fd < 0) {
                warnx("logfile opener: %s", strerror(errno));
                exit(1);
            }
        }
        else {
            log_file_fd = open( log_file, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR );
            
            if ( log_file_fd < 0 ) {
                warnx("logfile opener: %s", strerror(errno));
                exit(1);
            }
        }
    }

    // Getting Port Number
    // First Checking there is an actual number available
    if ( optind == argc ) {
        warnx( "No port given: ./httpserver [-t threads] [-l logfile] <port>" );
        exit( 1 );
    }

    uint16_t port_num = StrToPort( argv[ optind ] );
    int socket_fd = create_listen_socket( port_num );

    if ( socket_fd < 0 ) {
        warnx( "bind: %s", strerror( errno ) );
        exit( 1 );
    }

    // Initializing queue for requests
    Queue *client_queue = QueueNew( number_of_threads ); // creates a queue size of threads

    // Initializing file locks
    FileLocks *file_locks = CreateFileLocks( number_of_threads );

    // Setting up threads
    FinishedThreadsInit( number_of_threads );

    pthread_t worker_threads[number_of_threads];

    int thread_numbers[ number_of_threads ];

    for ( int i = 0; i < number_of_threads; ++i ) {
        thread_numbers[ i ] = i;
    }

    for ( int i = 0; i < number_of_threads; ++i ) {
        void *ThreadArg = &( thread_numbers[i] ); // Giving each thread an identifier
        pthread_create( worker_threads + i, NULL, WorkerRequest, ThreadArg );
    }

    while ( !ev_interrupt_received ) {
        int *client_fd = malloc( sizeof(int) );
        *( client_fd ) = accept( socket_fd, NULL, NULL ); // waiting for connections

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
        if ( !QueuePush(  client_queue , client_fd ) ) {
            free( client_fd );
        }
    }

    bool all_threads_finished = false;
    QueueShutDown( client_queue );

    // Sending to command to queue to wake up threads
    while (!all_threads_finished) {
        int numFin = 0; // Shows how many threads finished
        for (int i = 0; i < g_SignalThreads->size; ++i) {
            if (g_SignalThreads->finishThreads[i]) {
                ++numFin; // Increment counter for finish
            }
        }

        // Checking numFin
        if (numFin == g_SignalThreads->size)
            all_threads_finished = true; // If all threads finish set to notify
        else
            usleep(100); // sleep for 100 microseconds
    }

    // Joining threads
    for (int i = 0; i < number_of_threads; ++i) {
        pthread_join(worker_threads[i], NULL);
    }

    printf("End of Handling Requests\n");

    // Removing queue structs present
    while (queue_size( client_queue ) != 0) {
        void *garbage_val;
        QueuePop( client_queue , &garbage_val );
        free( garabge_val );
    }

    QueueDelete( &client_queeu );
    FinishedThreadsInitFree();
    DeleteFileLocks( &file_locks );
    fsync( log_file_fd ); //** Freeing memory for URI lock
    close( log_file_fd ); // Close log file
    return 0;
}

uint16_t StrToPort(char *port) {
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

        if ( !QueuePop(  client_queue , &client_temp ) ) {
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
, Content_length, Request_ID, log_file_fd);

            }
            // GET
            else if (Meth == GET)
                resultMeth = http_methods_GetReq(fileName, client_fd, Request_ID, log_file_fd);
            // HEAD
            else
                resultMeth = http_methods_HeadReq(fileName, client_fd, Request_ID, log_file_fd);
        }
        /*Resetting all variables to original values*/
        close(client_fd);
    }

    g_SignalThreads->finishThreads[threadNum] = true;
    return NULL;
}


void FinishedThreadsInit( int number_of_thr eads) {
    g_SignalThreads = malloc(sizeof(SignalThreads)); // malloc array for singal threads
    g_SignalThreads->size = number_of_threads; // Holds How many threads present
    g_SignalThreads->finishThreads
        = calloc(number_of_threads, sizeof(atomic_bool)); // Initializing boolean array
}

void FinishedThreadsInitF ree() {
    free(g_SignalThreads->finishThreads); // free bool array
    free(g_SignalThreads);
    g_SignalThreads = NULL;
}

void Opt ionalArgumentChecker(
    bool *l_flag, bool *t_flag, int argc, char *argv[], int *number_of_threads, char *log_file) {
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
                *number_of_threads = atoi(optarg);
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
