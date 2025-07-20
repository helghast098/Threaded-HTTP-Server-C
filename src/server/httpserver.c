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

int log_file_fd = STDERR_FILENO; // default logFile Descriptor

atomic_bool g_interrupt_received = false;
void SignalInterrupt(int signum) {
    if (signum == SIGTERM) {
        g_interrupt_received = true;
    }
}

typedef struct ThreadsFinished {
    const int num_of_threads;
    atomic_int num_of_threads_finished;
} ThreadsFinished;

typedef struct ThreadArguments {
    FileLocks *const file_locks;
    ThreadsFinished *status_of_threads;
    Queue *client_queue;
} ThreadArguments;

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

void ListenForClients( Queue *client_queue ) {
    while ( !g_interrupt_received ) {
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
}

void OptionalArgumentChecker( bool *l_flag, bool *t_flag, int argc, char *argv[], int *number_of_threads, char *log_file ) {
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

void *WorkerRequest( void *arg ) {
    ThreadArguments * thread_arg = arg ;

    Buffer *client_buffer = CreateBuffer( CLIENT_BUFFER_SIZE );

    Request * request = CreateRequest( REQUEST_BUFFER_SIZE, FILE_NAME_LENGTH );

    while ( !ev_interrupt_received ) {
        bool client_closed = false;
        void *client_temp = NULL;
        int client_fd;

        // reseting variables
        client_closed = false;

        request.buffer.length = 0;
        request.current_index = 0;
        request.current_state = INITIAL_STATE;
        request.type = NOT_VALID;
        request.headers.content_length = -1;
        request.headers.request_id = -1;
        request.headers.expect = false;

        if ( !QueuePop(  thread_arg->client_queue , &client_temp ) ) {
            break;
        }

        client_fd = *( ( int * ) client_temp );
        free( client_temp );

        // need to set client to nonblocking
        fcntl( client_fd, F_SETFL, O_NONBLOCK );

        while ( !g_interrupt_received && ( request.current_state != REQUEST_COMPLETE ) ) {
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

        if ( g_interrupt_received ) {
            close( client_fd );
            break;
        }

        if ( !client_closed ) {

            // CHECKING FORMAT OF REQUEST
            // Checking Request
            if ( RequestChecker( &request ) != 0 ) {
                StatusPrint( client_fd, BAD_REQUEST_ );
                close( client_fd );
                continue;
            }

            // Checking headerfields
            if ( HeaderFieldChecker( &request ) != 0 ) {
                StatusPrint( client_fd, BAD_REQUEST_ );
                close( client_fd );
                continue;
            }

            // CHOOSING METHOD
            switch ( request.type ) {
                case HEAD:
                    HeadOrGetRequest( &request, &client_buffer, client_fd, log_file_fd, &g_interrupt_received, thread_arg->file_locks );
                break;

                case GET:
                    HeadOrGetRequest( &request, &client_buffer, client_fd, log_file_fd, &g_interrupt_received, thread_arg->file_locks );
                break;

                case PUT:
                    PUTRequest( &request, &client_buffer, client_fd, log_file_fd, &g_interrupt_received, thread_arg->file_locks );
                break;

                default:
                    StatusPrint( client_fd, NOT_IMP_ );
            }
        }
        /*Resetting all variables to original values*/
        close(client_fd);
    }
    ++( thread_arg->status_of_threads->num_of_threads_finished );

    free( thread_arg );
    DeleteBuffer( &client_buffer );
    DeleteRequest( &request );

    return NULL;
}

int main(int argc, char *argv[]) {
    // SETTING UP INTERRUPT STRUCT
    struct sigaction interrupt_signal;
    memset( &interrupt_signal, 0, sizeof( interrupt_signal ) );
    interrup_signal.sa_handler = SignalInterrupt;
    sigaction( SIGTERM, &interrupt_signal, NULL );

    //FLAGS FOR OPTIONAL ARGUMENTS
    bool l_flag = false;
    bool t_flag = false;
    int number_of_threads = 4;

    // IF THERE ARE NO ARGUMENTS
    if (argc < 2) {
        warnx("Too few arguments:\nusage: ./httpserver [-t threads] [-l logfile] <port>");
        exit(1);
    }

    // CHECKING OPTIONAL ARGUMENTS
    OptionalArgumentChecker( &l_flag, &t_flag, argc, argv, &number_of_threads, log_file );

    // OPENING LOG FILE
    char log_file[ 1000 ];

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

    // GETTING PORT NUMBER
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

    // INITIALIZING QUEUE FOR REQUESTS
    Queue *client_queue; = QueueNew( number_of_threads );

    // INITIALIZING FILE LOCKS
    FileLocks *file_locks = CreateFileLocks( number_of_threads );

    // SETTING UP THREADS
    ThreadsFinished status_of_threads;
    status_of_threads.num_of_threads = number_of_threads;
    status_of_threads.num_of_threads_finished = 0;

    pthread_t worker_threads[ number_of_threads ];

    for ( int i = 0; i < number_of_threads; ++i ) {
        ThreadArguments *thread_arg = malloc( sizeof( ThreadArguments ) );
        thread_arg->status_of_threads = &file_locks;
        thread_arg->status_of_threads = &status_of_threads;
        thread_arg->client_queue = client_queue;

        pthread_create( worker_threads + i, NULL, WorkerRequest, ( void * ) thread_arg );
    }

    // ACQUIRING CLIENTS
    ListenForClients( client_queue );

    // SHUTDOWN SERVER
    QueueShutDown( client_queue );

    while ( status_of_threads.num_of_threads != status_of_threads.num_of_threads_finished ) {
        usleep(100); // sleep for 100 microseconds
    }

    for (int i = 0; i < number_of_threads; ++i) {
        pthread_join( worker_threads[ i ], NULL );
    }

    // Removing queue structs present
    while ( queue_size( client_queue ) != 0 ) {
        void *garbage_val;
        QueuePop( client_queue , &garbage_val );
        free( garabge_val );
    }

    QueueDelete( &client_queeu );
    DeleteFileLocks( &file_locks );

    fsync( log_file_fd ); //** Freeing memory for URI lock
    close( log_file_fd ); // Close log file
    return 0;
}