/**
* Created By Fabert Charles
*
* Houses function declarations for request_format
*/

#ifndef REQUEST_FORMAT_H
#define REQUEST_FORMAT_H

/*Libraries Included*/
#include <stdbool.h>

/*Macro Definitions*/
#define METHOD_LENGTH     8
#define VERSION_LENGTH 8

/*Type Definitions*/
typedef enum { HEAD, PUT, GET, NO_VALID } Methods;

/*Function Declarations*/
/** @brief Checks if the request from the client is calid
*   @param buffer: Holds the string of the request
*   @param currentPos: Current position of buffer to start
*   @param bufferSize: Size of buffer
*   @param file: Holds the filename the client requested
*   @param meth: The method (put, get, head) the client wants to do
*   @return 0 for success, -1 on failure
*/
int request_format_RequestChecker(char* buffer, int* currentPos, int bufferSize, char* file, Methods* method);

/** @brief Checks if header fields are valid
*   @param buffer: Holds the string of the request
*   @param currentPos: Current position of buffer to start
*   @param bufferSize: Size of buffer
*   @param contLength: Where the content length is put that is found from the buffer (request)
*   @param reqID: Holds the request ID headerfield value
*   @param method: The type of request from client
*   @return 0 on success, -1 on failure 
*/
int request_format_HeaderFieldChecker(char* buffer, int* currentPos, int bufferSize, long int* contLength, long int* reqID, Methods* method);

#endif
