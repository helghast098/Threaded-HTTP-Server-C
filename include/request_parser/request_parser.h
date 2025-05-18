/**
* Created By Fabert Charles
*
* Houses function declarations for request_format
*/

#ifndef SIMPLEHTTP_REQUEST_PARSER_H_
#define SIMPLEHTTP_REQUEST_PARSER_H_

/*Libraries Included*/
#include <stdbool.h>

/*Macro Definitions*/
#define METHOD_LENGTH  8
#define FILE_NAME_LENGTH 1000 // Not including \0
#define VERSION_LENGTH 8
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
    Buffer buffer;
    RequestState current_state;
    Methods type; // Set by RequestChecker
    char *file // Set by RequestChecker() please malloc the data
} Request;

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
int HeaderFieldChecker(int clientFD, char* buffer, int* currentPos, int bufferSize, long int* contLength, long int* reqID, Methods* method);

#endif
