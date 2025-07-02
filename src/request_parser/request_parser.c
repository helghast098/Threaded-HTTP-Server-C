/**
* Created by Fabert Charles
*
* Houses function definitions for request_format
*/

#include "request_parser/request_parser.h"

/*Libraries Included*/
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
} HeaderState;

typedef enum {
    STATE_CHECK_METHOD=0,
    STATE_CHECK_FILE_PATH,
    STATE_CHECK_HTTP_VERSION,
    STATE_VALID_REQUEST
} RequestCheckerState;

// Helper Functions
bool ValidMethodChar( char c ) {
    return ( ( c >= 'A' && c <= 'Z' ) || ( c == ' ' ) );
}

bool ValidFileChar( char c ) {
    return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) || ( c == '.' )
            || ( c == '_' ) || ( c == ' ' ) || ( c == '/' );
}

bool ValidValKeyChar( char c ) {
    return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c == '-' ) || ( c == '_' ) || ( c >= '0' && c <= '9' );
}

void HeaderValCheck( Request *request, char *key, char *val ) {
    // Content-Length
    if ( strcmp( key, "Content-Length" ) == 0 ) {
        request->headers.content_length = strtol( val, NULL, 10 );
    }
    // Request-Id
    else if ( strcmp( key, "Request-Id" ) == 0 ) {
        request->headers.request_id = strtol( val, NULL, 10 );
    }
    // Expect
    else if ( strcmp( key, "Expect" ) == 0 ) {
        // Write now accepting any size file.
        if ( strcmp( val, "100-continue" ) == 0 ) {
            request->headers.expect = true;
        }
    }
}

void RemoveLeftTrailingSpaces( Request *request ) {
    int i = request->request_string.current_index;
    while ( ( i < request->request_string.length ) && ( request->request_string.data[ i ] == ' ') ) {
        ++( request->request_string.current_index );
        i = request->request_string.current_index;
    }

}

int GetMethod( Request *request ) {
    char method[10]; // holds method part
    // Checking If method is valid
    for ( int i = 0; i < METHOD_LENGTH + 1; ++i ) {
        char c = request->request_string.data[i];
        ++( request->request_string.current_index );
        if ( !ValidMethodChar( c ) || ( i == METHOD_LENGTH && c != ' ' ) ) {
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
    size_t start_index = request->request_string.current_index;
    int i = start_index;
    char c;
    char prev_char = '\0';
    while ( ( request->request_string.current_index < request->request_string.length ) && ( ( i - start_index ) <= ( FILE_NAME_LENGTH + 1) ) ) {
        c = request->request_string.data[ i ];

        if ( ( prev_char == '/' ) && ( prev_char == c ) ) {
            return -1;
        }

        if ( ( i == start_index ) ) {
            if  ( request->request_string.data[ start_index ] != '/' ) {
                return -1;
            }
        }
        else if ( !ValidFileChar( c ) ) {
            return -1;
        }
        else if ( c == ' ' ) {
            request->file[ i - start_index - 1 ] = '\0';

            if ( ( i - start_index ) ==  1 )
            {
                return -1;
            }
            ++( request->request_string.current_index );
            return 0;
        }
        else {
            if ( ( i - start_index ) == ( FILE_NAME_LENGTH + 1 ) ) {
                return -1;
            }
            (*request).file[ i - start_index - 1] = c;
        }
        ++( request->request_string.current_index );
        i = request->request_string.current_index;
        prev_char = c;
    }

    return -1;

} 

int CheckHTTPVersion( Request *request ) {

    char http_version[ VERSION_LENGTH + 1 ];

    int start_index = request->request_string.current_index;
    int i = start_index;

    while ( request->request_string.current_index < request->request_string.length && ( i - start_index ) < VERSION_LENGTH ) {

        char c = request->request_string.data[ i ];

        http_version[ i - start_index ] = c;

        ++ ( request->request_string.current_index );
        i = request->request_string.current_index;
    }

    http_version[ i - start_index] = '\0';

    if ( strcmp( "HTTP/1.1\r\n", http_version ) != 0 ) {
        return -1;
    }
    return 0;
}

// Main Functions
int RequestChecker( Request *request) {
    RequestCheckerState state = STATE_CHECK_METHOD;
    request->request_string.current_index = 0;

    while ( state != STATE_VALID_REQUEST ) {
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

int HeaderFieldChecker( Request *request ) {
    HeaderState current_state = STATE_START;

    char *key = malloc( sizeof ( char ) * KEY_AND_VAL_MAX_LENGTH );
    size_t key_index = 0;

    char *val = malloc ( sizeof( char ) * KEY_AND_VAL_MAX_LENGTH );
    size_t val_index = 0;

    bool content_length_found = false;

    while ( request->request_string.current_index < request->request_string.length ) {
        int i = request->request_string.current_index;

        switch ( current_state ) {
            case STATE_START:
                if ( request->request_string.data[ i ] == '\r' ) {
                    ++ ( request->request_string.current_index );
                    current_state = STATE_2_R;
                }
                else if ( request->request_string.data[ i ] == '\n' ) {
                    return -1;
                }
                else {
                    current_state = STATE_START_KEY;
                }
            break;

            case STATE_START_KEY:
                key_index = 0;
                val_index = 0;

                if ( request->request_string.data[ i ] == ' ' ) {
                    RemoveLeftTrailingSpaces( request );
                }
                else if (  !ValidValKeyChar( request->request_string.data[ i ] ) ) {
                    return -1;
                }
                else {
                    key[ key_index ] = request->request_string.data[ i ];
                    ++key_index;
                    ++ ( request->request_string.current_index );
                    current_state = STATE_KEY;
                }
            break;

            case STATE_KEY:
                if ( key_index >= KEY_AND_VAL_MAX_LENGTH ) {
                    return -1;
                }

                if ( request->request_string.data[ i ] == ':' ) {
                    key[ key_index ] = '\0';
                    ++ ( request->request_string.current_index );
                    current_state = STATE_START_VAL;
                }
                else if ( !ValidValKeyChar( request->request_string.data[ i ] ) ) {
                    return -1;
                }
                else {
                    key[ key_index ] = request->request_string.data[ i ];
                    ++key_index;
                    ++ ( request->request_string.current_index );
                }
            break;

            case STATE_START_VAL:
                if ( request->request_string.data[ i ] == ' ' ) {
                    RemoveLeftTrailingSpaces( request );
                }
                else if (  !ValidValKeyChar( request->request_string.data[ i ] ) ) {
                    return -1;
                }
                else {
                    val[ val_index ] = request->request_string.data[ i ];
                    ++val_index;
                    ++ ( request->request_string.current_index );
                    current_state = STATE_VAL;
                }
            break;

            case STATE_VAL:
                if (  val_index >=  KEY_AND_VAL_MAX_LENGTH ) {
                    return -1;
                }

                if ( request->request_string.data[ i ] == '\r' ) {
                    val[ val_index ] = '\0';
                    ++( request->request_string.current_index );
                    current_state = STATE_1_R;
                }
                else if ( !ValidValKeyChar( request->request_string.data[ i ] ) ) {
                    return -1;
                }
                else {
                    val [ val_index  ] = request->request_string.data[ i ];
                    ++val_index;
                    ++( request->request_string.current_index );
                } 
            break;

            case STATE_1_R:
                if ( request->request_string.data[ i ] != '\n' ) {
                    return -1;
                }

                ++( request->request_string.current_index );
                current_state = STATE_1_N;
            break;

            case STATE_1_N:
                HeaderValCheck( request, key, val );

                if ( request->request_string.data[ i ] != '\r' ) {
                    current_state = STATE_START_KEY;
                }
                else {
                    current_state = STATE_2_R;
                    ++ ( request->request_string.current_index );
                }
            break;

            case STATE_2_R:
                if ( request->request_string.data[ i ] != '\n' ) {
                    return -1;
                }
                
                if ( ( request->type == PUT ) && ( request->headers.content_length == -1 ) ) {
                    return -1;
                }
                ++ ( request->request_string.current_index );
                return 0;

            break;

            default:
                return -1;
        }
    }
    return -1;
}
