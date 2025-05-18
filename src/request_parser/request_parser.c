/**
* Created by Fabert Charles
*
* Houses function definitions for request_format
*/

#include "request_parser/request_parser.h"

/*Libraries Included*/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef enum {
    STATE_START=0,
    STATE_FAILURE,
    STATE_START_KEY,
    STATE_KEY,
    STATE_START_VAL,
    STATE_VAL,
    STATE_1_R,
    STATE_1_N,
    STATE_2_R
} Header_State;

typedef enum {
    STATE_CHECK_METHOD=0,
    STATE_CHECK_FILE_PATH,
    STATE_CHECK_HTTP_VERSION,
    STATE_VALID_REQUEST
} RequestCheckerState;

bool ValidChar( char c ) {
    return ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c == ' ' ) );
}

bool ValidFileChar( char c ) {
    return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) || ( c == '.' )
            || ( c == '_' ) || ( c == ' ' ) || ( c == '/' )
}

int GetMethod( Request *request ) {
    char method[10]; // holds method part

    // Checking If method is valid
    for ( int i = 0; i < METHOD_LENGTH + 1; ++i ) {
        char c = request->buffer.data[i];
        ++( request->buffer.current_index );

        if ( !ValidChar( c ) || ( i == METHOD_LENGTH && c != ' ' ) ) {
            return -1;
        }
        else if (c == ' ') {
            method[i] = '\0';
            break;
        }
        else {
            method[i] = c;
        }
    }

    // Determining Method
    if ( strcmp( "HEAD", method ) == 0 ) {
        request->type = HEAD;
    }
    else if ( strcmp( "GET", method ) == 0 ) {
        request->type = GET;
    }
    else if ( strcmp( "PUT", method ) == 0 ) {
        request->type = PUT;
    }
    else {
        request->type = NOT_VALID;
        return -1;
    }
    return 0;
}

int GetFilePath( Request *request ) {
    size_t start_index = request->buffer.current_index;
    int i = start_index;

    while ( request->buffer.current_index < request->buffer.length && ( i - start_index  ) <= FILE_NAME_LENGTH ) {
        char c = request->buffer.data[ i ];
        i = request->buffer.current_index;
        ++( request->buffer.current_index );

        if ( ( i == start_index ) ) {
            if  ( request->buffer.data[ start_index ] != '/' ) {
                return -1;
            }
        }
        else if ( !ValidChar( c ) ) {
            return -1;
        }
        else if ( c == ' ' ) {
            request->file[ i - start_index ] = '\0';
            return 0;
        }
        else {
            request->file[ i - start_index ] = c;
        }
    }

    return -1;

} 

int CheckHTTPVersion( Request *request ) {

    char http_version[ VERSION_LENGTH + 1 ];

    int start_index = request->buffer.current_index;
    int i = start_index;

    while ( i < request->buffer.length &&  ( i - start_index ) < VERSION_LENGTH ) {
        i = request->buffer.current_index;
        ++ ( request->buffer.current_index );

        char c = request->buffer.data[ i ];

        http_version[ i - start_index ] = c;
    }

    http_version[ i - start_index + 1] = '\0';

    if ( strcmp( "HTTP/1.1", http_version ) != 0 ) {
        return -1;
    }
    return 0;

}

int RequestChecker( Request *request) {
    RequestCheckerState state = STATE_CHECK_METHOD;
    request->buffer.current_index = 0;
    request->current_state = STATE_CHECK_METHOD;

    while ( request->buffer.current_index < request->buffer.length && ( state != STATE_VALID_REQUEST ) ) {
        switch ( state ) {
            case STATE_CHECK_METHOD:
                if ( GetMethod( request ) == -1 ) {
                    return -1;
                }
                state = STATE_CHECK_FILE_PATH;
            break;
            
            case STATE_CHECK_FILE_PATH:
                if ( GetFilePath( request ) == -1 ) {
                    return -2;
                }
                state = STATE_CHECK_HTTP_VERSION;
            break;

            case STATE_CHECK_HTTP_VERSION:
                if ( CheckHTTPVersion( request ) == -1 ) {
                    return -3;
                }
                state = STATE_VALID_REQUEST;
            break;

            default:
                return -4;    
        }
    } 
    return 0;
}

/*Function Definitions*/

int HeaderFieldChecker(int clientFD, char *buffer, int *currentPos, int bufferSize,
    long int *contLength, long int *reqID, Methods *method) {
    Header_State currentState = STATE_START;

    char key[1000];
    int key_indx = 0;

    char val[1000];
    int val_indx = 0;

    bool content_length_found = false;

    while (*currentPos != bufferSize) {
        switch (currentState) {
        case STATE_START:
            if (buffer[*currentPos] == '\r') {
                *currentPos += 1;
                currentState = STATE_2_R;
            } else if (buffer[*currentPos] == '\n') {
                return -1;
            } else {
                currentState = STATE_START_KEY;
            }
            break;

        case STATE_START_KEY:
            // remove all white spaces
            if (buffer[*currentPos] == ' ') {
                while (*currentPos != bufferSize && buffer[*currentPos] == ' ') {
                    *currentPos += 1;
                }
            } else if (buffer[*currentPos] == '\r' || buffer[*currentPos] == '\n'
                       || buffer[*currentPos] == ':') {
                return -1;
            } else {
                currentState = STATE_KEY;
            }
            break;

        case STATE_KEY:
            if (buffer[*currentPos] == ':') {
                key[key_indx] = '\0';
                *currentPos += 1;
                currentState = STATE_START_VAL;
            } else if (buffer[*currentPos] == ' ' || buffer[*currentPos] == '\r'
                       || buffer[*currentPos] == '\n') {
                return -1;
            } else {
                key[key_indx] = buffer[*currentPos];
                key_indx += 1;
                *currentPos += 1;
            }
            break;

        case STATE_START_VAL:
            // remove all white spaces
            if (buffer[*currentPos] == ' ') {
                while (*currentPos != bufferSize && buffer[*currentPos] == ' ') {
                    *currentPos += 1;
                }
            } else if (buffer[*currentPos] == ' ' || buffer[*currentPos] == '\r'
                       || buffer[*currentPos] == '\n') {
                return -1;
            } else {
                currentState = STATE_VAL;
            }
            break;

        case STATE_VAL:
            if (buffer[*currentPos] == '\r') {
                val[val_indx] = '\0';
                *currentPos += 1;
                currentState = STATE_1_R;
            } else if (buffer[*currentPos] == ' ' || buffer[*currentPos] == '\n') {
                return -1;
            } else {
                val[val_indx] = buffer[*currentPos];
                *currentPos += 1;
                val_indx += 1;
            }
            break;

        case STATE_1_R:
            if (buffer[*currentPos] != '\n') {
                return -1;
            }

            currentState = STATE_1_N;
            *currentPos += 1;
            break;

        case STATE_1_N:

            // Checking for key
            if (key_indx != 0) {
                // Content-Length
                if (strcmp(key, "Content-Length") == 0) {
                    *contLength = strtol(val, NULL, 10);
                    content_length_found = true;
                }
                // Request-Id
                else if (strcmp(key, "Request-Id") == 0) {
                    *reqID = strtol(val, NULL, 10);
                }
                // Expect
                else if (strcmp(key, "Expect") == 0) {
                    // Write now accepting any size file.
                    if (strcmp(val, "100-continue") == 0) {
                        if (write(clientFD, "HTTP/1.1 100 Continue\r\n\r\n", 29) < 0) {
                            return -1;
                        }
                    }
                }
            }

            if (buffer[*currentPos] != '\r') {
                currentState = STATE_START_KEY;
            } else {
                currentState = STATE_2_R;
                *currentPos += 1;
            }

            key_indx = 0;
            val_indx = 0;
            break;

        case STATE_2_R:
            if (buffer[*currentPos] != '\n') {
                return -1;
            }
            *currentPos += 1;

            if (*method == PUT && !content_length_found) {
                return -1;
            }
            return 0;
        }
    }

    return -1;
}
