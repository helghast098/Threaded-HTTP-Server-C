/**
* Created By Fabert Charles
*
* Houses function declarations for request_format
*/

#ifndef SIMPLEHTTP_REQUEST_PARSER_H_
#define SIMPLEHTTP_REQUEST_PARSER_H_

/*Libraries Included*/
#include <stdbool.h>
#include <stdlib.h>

/*Macro Definitions*/
#define METHOD_LENGTH  10
#define FILE_NAME_LENGTH 1000 // Not including \0
#define VERSION_LENGTH 10
#define KEY_AND_VAL_MAX_LENGTH 1000

/*Type Definitions*/
typedef enum { HEAD, PUT, GET, NOT_VALID } Methods;

typedef enum {
    INITIAL_STATE = 0,
    FIRST_R,
    FIRST_N,
    SECOND_R,
    REQUEST_COMPLETE
} RequestState;

typedef struct {
    char *data;
    size_t current_index;
    size_t length;
    size_t max_size;
} Buffer;

typedef struct {
    long int content_length;
    long int request_id;
    bool expect;
} Headers;

typedef struct {
    Buffer request_string;
    RequestState current_state; // Used to determine completion of request
    Methods type; // Set by RequestChecker
    Headers headers; // set by header field checker
    char *file; // Set by RequestChecker please malloc the data
} Request;


Buffer *CreateBuffer( size_t size );

void DeleteBuffer( Buffer **buff );

Request *CreateRequest( size_t size );

void DeleteRequest( Request **req );



/*Function Declarations*/
/** @brief Checks if the request from the client is valid
*   @param buffer: Holds the string of the request
*   @param currentPos: Current position of buffer to start
*   @param bufferSize: Size of buffer
*   @param file: Holds the filename the client requested
*   @param meth: The method (put, get, head) the client wants to do
*   @return 0 for success, -1 on failure
*/

int RequestChecker(Request *request);

/** @brief Checks if header fields are valid
*   @param buffer: Holds the string of the request
*   @param currentPos: Current position of buffer to start
*   @param bufferSize: Size of buffer
*   @param contLength: Where the content length is put that is found from the buffer (request)
*   @param reqID: Holds the request ID headerfield value
*   @param method: The type of request from client
*   @return 0 on success, -1 on failure
*/

int HeaderFieldChecker( Request *request );
#endif
