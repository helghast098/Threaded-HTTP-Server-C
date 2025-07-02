/**
* Created by Fabert Charles
*
*/

#include "http_methods/http_methods.h"
#include "request_parser/request_parser.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>

#define MAX_PUT_MESSAGE_BUFF 10000
#define MAX_GET_MESSAGE_BUFFER 4096


// Helper Functions
int WriteToClient( Buffer *data, int client_fd, atomic_bool *interrupt ) {
    while ( data->current_index < data->length ) {
        size_t remain_bytes_to_write = data->length - data->current_index;
        char *message_start = data->data + data->current_index;

        ssize_t bytes_written = write( client_fd, message_start, remain_bytes_to_write );
        if ( ( bytes_written < 0 ) || interrupt ) {
            return -1;
        }  

        data->current_index += bytes_written;
    }
    return 0;
}

// Function Definitions
int PutRequest( Request *request, Buffer *client_buffer, int client_fd, int log_fd, atomic_bool *interrupt_received, FileLocks *file_locks ) {
    char temp_file[] = "TEMPFILE-XXXXXX";
    int temp_fd = mkstemp( temp_file );

    Buffer file_write_buffer = {
        .data = malloc( MAX_PUT_MESSAGE_BUFF ),
        .length = 0,
        .max_size = MAX_PUT_MESSAGE_BUFF,
        .current_index = 0
    };
    
    ssize_t bytes_written; // used for output of write()
    size_t bytes_left_to_write = request->headers.content_length;
    size_t bytes_left_in_client_buffer = client_buffer->length - client_buffer->current_index;


    for ( int i = 0; ( i < bytes_left_in_client_buffer ) && ( bytes_left_to_write != 0 ); ++i ) {
        file_write_buffer.data[ i ] = client_buffer->data[ client_buffer->current_index ];

        ++( file_write_buffer.current_index );
        ++( file_write_buffer.length );

        ++( client_buffer->current_index );

        --bytes_left_to_write;
    }
    
    if ( bytes_left_to_write == 0 ) {
        bytes_written = write(  temp_fd, file_write_buffer.data, request->headers.content_length );
    }
    else {
        while ( ( bytes_left_to_write > 0 ) && !interrupt_received ) {
            // write to file
            if ( file_write_buffer.length == MAX_PUT_MESSAGE_BUFF ) {
                bytes_written = write( temp_fd, file_write_buffer.data, file_write_buffer.length );
                if ( bytes_written < 0) {
                    break;
                }

                file_write_buffer.current_index = 0;
                file_write_buffer.length = 0;
            }
            // read from client
            int bytes_read_client = read( client_fd, file_write_buffer.data + file_write_buffer.current_index,
                MAX_PUT_MESSAGE_BUFF - file_write_buffer.current_index );
            
            if ( bytes_read_client <= 0 ) {
                break;
            }

            file_write_buffer.current_index += bytes_read_client;
            file_write_buffer.length +=bytes_read_client;
            --bytes_left_to_write;
        }

        if ( bytes_left_to_write == 0 && !interrupt_received ) {
            bytes_written = write( temp_fd, file_write_buffer.data, file_write_buffer.length );
        }
        else {
            StatusPrint(client_fd, ISE_);
            LogFilePrint(request->headers.request_id, log_fd, 500, request->file, "PUT"); // Printing to Log
        }
    }
    close( temp_fd );

    FileLink acquired_file_lock = LockFile( file_locks, request->file, WRITE );

    // CRITICAL SECTION

    
    rename( temp_file, request->file );

    /*If I cannot process the full request because of some interruption*/
    if ( bytes_left_to_write != 0 ) {
        // clear file
        int fd = open( request->file, O_TRUNC | O_WRONLY );
        close( fd );
        
        free( file_write_buffer.data );
        return -1;
    }

    // success and Request Printing
    if ( access( request->file, F_OK ) != 0 ) {
        StatusPrint( client_fd, CREATED_ );
        LogFilePrint( request->headers.request_id, log_fd, 201, request->file, "PUT" ); // Printing to Log
    } else {
        StatusPrint( client_fd, OK_ );
        LogFilePrint( request->headers.request_id, log_fd, 200, request->file, "PUT" ); // Printing to Log
    }

    // END CRITICAL SECTION
    UnlockFile( file_locks, &acquired_file_lock );

    return 0;
}

int GetRequest( Request *request , Buffer *client_buffer, int client_fd, int log_fd, atomic_bool *interrupt_received, FileLocks *file_locks ) {
    int fd;
    struct stat file_stats;

    // Acquire URI Lock for file

    FileLink acquired_file_lock = LockFile( file_locks, request->file, READ );

    // CRITICAL SECTION

    // Checking if file exists, opens it, and if its a directory
    if ( access( request->file, F_OK ) == 0 ) {
        bool error_with_file = true;

        fd = open( request->file, O_RDONLY );

        if ( fd >= 0  ) {
            fstat( fd, &file_stats );

            if ( S_ISDIR( file_stats.st_mode ) ) {
                close( fd );
            }
            else {
                error_with_file = false;
            }
        }

        if ( error_with_file ) {
            UnlockFile( file_locks, &acquired_file_lock );
            StatusPrint( client_fd, ISE_ );
            LogFilePrint( request->headers.request_id , log_fd, 500, request->file, "GET" );
            return -1;
        }
    }
    else {
        StatusPrint( client_fd, NOT_FOUND_ );

        LogFilePrint( request->headers.request_id, log_fd, 404, request->file, "GET");

        UnlockFile( file_locks, &acquired_file_lock );
        return -1;
    }

    // send content length to client
    Buffer message_str = {
        .data = malloc( sizeof( char ) * 512 ),
        .current_index = 0,
        .length = 0,
        .max_size = 512
    };

    sprintf( message_str.data , "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", file_stats.st_size );
    message_str.length = strlen( message_str.data );

    if ( WriteToClient( &message_str, client_fd, interrupt_received ) == -1 ) {
        free( message_str.data );
        close( fd );
        UnlockFile( file_locks, &acquired_file_lock );
        StatusPrint(client_fd, ISE_);
        LogFilePrint( request->headers.request_id, log_fd, 500, request->file, "GET" );
        return -1;
    }
    free( message_str.data );

    // Writing content to client
    Buffer buffer = {
        .data = malloc( sizeof( char ) *  MAX_GET_MESSAGE_BUFFER ),
        .current_index = 0,
        .length = 0,
        .max_size = MAX_GET_MESSAGE_BUFFER
    };

    size_t bytes_needed_to_send = file_stats.st_size;

    bool error = false;

    while ( bytes_needed_to_send != 0 ) {
        // reading from file
        buffer.current_index = 0;
        buffer.length = 0;
        ssize_t bytes_read_from_file = read( fd, buffer.data, buffer.max_size );

        if ( bytes_read_from_file < 0 || interrupt_received ) {
            error = true;
            break;
        }
        buffer.length = bytes_read_from_file;

        // sending data to client
        if ( WriteToClient( &buffer, client_fd, interrupt_received ) == -1 ) {
            error = true;
            break;
        }

        bytes_needed_to_send -= bytes_read_from_file;
    }

    if ( error ) {
        free( buffer.data );
        StatusPrint( client_fd, ISE_ );
        LogFilePrint( request->headers.request_id, log_fd, 500, request->file, "GET" );
        UnlockFile( file_locks, &acquired_file_lock );
    }

    close(fd);
    UnlockFile( file_locks, &acquired_file_lock );
c asdf  a as
    // END CRITICAL SECTION
    LogFilePrint( request->headers.request_id, log_fd, 200, request->file, "GET" );
    free( buffer.data );
    return 0;
}

int HeadRequest( Request *request , Buffer *client_buffer, int client_fd, int log_fd, atomic_bool *interrupt_received, FileLocks *file_locks ) {
    struct stat file_stats;
    int fd;
    FileLink acquired_file_lock = LockFile( file_locks, request->file, READ );

    // START CRITICAL SECTION

     // Checking if file exists, opens it, and if its a directory
    if ( access( request->file, F_OK ) == 0 ) {
        bool error_with_file = true;

        fd = open( request->file, O_RDONLY );

        if ( fd >= 0  ) {
            fstat( fd, &file_stats );

            if ( S_ISDIR( file_stats.st_mode ) ) {
                close( fd );
            }
            else {
                error_with_file = false;
            }
        }

        if ( error_with_file ) {
            UnlockFile( file_locks, &acquired_file_lock );
            StatusPrint( client_fd, ISE_ );
            LogFilePrint( request->headers.request_id , log_fd, 500, request->file, "HEAD" );
            return -1;
        }
    }
    else {
        StatusPrint( client_fd, NOT_FOUND_ );

        LogFilePrint( request->headers.request_id, log_fd, 404, request->file, "HEAD");

        UnlockFile( file_locks, &acquired_file_lock );
        return -1;
    }

    // read the data from file
    Buffer message_str = {
        .data = malloc( sizeof( char ) * 512 ),
        .current_index = 0,
        .length = 0,
        .max_size = 512
    };

    // send content length to client
    sprintf( message_str.data , "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", file_stats.st_size );
    message_str.length = strlen( message_str.data );

    if ( WriteToClient( &message_str, client_fd, interrupt_received ) == -1 ) {
        free( message_str.data );
        close( fd );
        UnlockFile( file_locks, &acquired_file_lock );
        StatusPrint(client_fd, ISE_);
        LogFilePrint( request->headers.request_id, log_fd, 500, request->file, "GET" );
        return -1;
    }

    close(fd);

    // END CRITICAL SECTION
    UnlockFile( file_locks, &acquired_file_lock );
    LogFilePrint( request->headers.request_id, log_fd, 200, request->file, "HEAD" );
    free( message_str.data );
    return 0;
}

void LogFilePrint( long int request_num, int log_fd, int status_code, char *file, char *method ) {
    // lock file
    while ( flock( log_fd, LOCK_EX ) < 0)
        ; // getting flock

    char log_message[ 2048 ];

    /*If the methods is PUT*/
    sprintf( log_message, "%s, %s, %d, %ld\n", method, file, status_code, request_num );

    write( log_fd, log_message, strlen( log_message ) );

    // unlock file
    flock( log_fd, LOCK_UN );
}

void StatusPrint(int client_fd, StatusC status) {
    int bytesWritten = 0; // Bytes written by write()
    int bytesContinuation = 0; // Continuing bytes for write()
    switch (status) {
    case OK_:
        do {
            bytesWritten
                = write(client_fd, "HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nOK\r\n", 42);
            // Increment bytes
            if (bytesWritten > 0) {
                bytesContinuation += bytesWritten;
            }
            // adding If not errno == INTR
            if ((bytesWritten < 0) && (errno != EINTR)) {
                break;
            }

        } while (((bytesWritten < 0) && (errno == EINTR))
                 || ((bytesContinuation < 42) && (bytesContinuation > 0)));
        break;

    case CREATED_:
        do {
            bytesWritten = write(
                client_fd, "HTTP/1.1 201 Created\r\nContent-Length: 9\r\n\r\nCreated\r\n", 52);
            // Increment bytes
            if (bytesWritten > 0) {
                bytesContinuation += bytesWritten;
            }
            // adding If not errno == INTR
            if ((bytesWritten < 0) && (errno != EINTR)) {
                break;
            }
        } while (((bytesWritten < 0) && (errno == EINTR))
                 || ((bytesContinuation < 52) && (bytesContinuation > 0)));
        break;

    case BAD_REQUEST_:
        do {
            bytesWritten = write(client_fd,
                "HTTP/1.1 400 Bad Request\r\nContent-Length: 13\r\n\r\nBad Request\r\n", 61);
            // Increment bytes
            if (bytesWritten > 0) {
                bytesContinuation += bytesWritten;
            }
            // adding If not errno == INTR
            if ((bytesWritten < 0) && (errno != EINTR)) {
                break;
            }
        } while (((bytesWritten < 0) && (errno == EINTR))
                 || ((bytesContinuation < 61) && (bytesContinuation > 0)));
        break;

    case FORBIDDEN_:
        do {
            bytesWritten = write(
                client_fd, "HTTP/1.1 403 Forbidden\r\nContent-Length: 11\r\n\r\nForbidden\r\n", 57);
            // Increment bytes
            if (bytesWritten > 0) {
                bytesContinuation += bytesWritten;
            }
            // adding If not errno == INTR
            if ((bytesWritten < 0) && (errno != EINTR)) {
                break;
            }
        } while (((bytesWritten < 0) && (errno == EINTR))
                 || ((bytesContinuation < 57) && (bytesContinuation > 0)));
        break;

    case NOT_IMP_:
        do {
            bytesWritten = write(client_fd,
                "HTTP/1.1 501 Not Implemented\r\nContent-Length: 17\r\n\r\nNot Implemented\r\n",
                69);
            // Increment bytes
            if (bytesWritten > 0) {
                bytesContinuation += bytesWritten;
            }
            // adding If not errno == INTR
            if ((bytesWritten < 0) && (errno != EINTR)) {
                break;
            }
        } while (((bytesWritten < 0) && (errno == EINTR))
                 || ((bytesContinuation < 69) && (bytesContinuation > 0)));
        break;

    case NOT_FOUND_:
        do {
            bytesWritten = write(
                client_fd, "HTTP/1.1 404 Not Found\r\nContent-Length: 11\r\n\r\nNot Found\r\n", 57);
            // Increment bytes
            if (bytesWritten > 0) {
                bytesContinuation += bytesWritten;
            }
            // adding If not errno == INTR
            if ((bytesWritten < 0) && (errno != EINTR)) {
                break;
            }
        } while (((bytesWritten < 0) && (errno == EINTR))
                 || ((bytesContinuation < 57) && (bytesContinuation > 0)));
        break;

    case ISE_:
        do {
            bytesWritten = write(client_fd,
                "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 23\r\n\r\nInternal Server "
                "Error\r\n",
                81);
            // Increment bytes
            if (bytesWritten > 0) {
                bytesContinuation += bytesWritten;
            }
            // adding If not errno == INTR
            if ((bytesWritten < 0) && (errno != EINTR)) {
                break;
            }
        } while (((bytesWritten < 0) && (errno == EINTR))
                 || ((bytesContinuation < 81) && (bytesContinuation > 0)));
        break;
    }
}
