/**
* Created by Fabert Charles
*
* Houses all functions for Http methods processing
*
*/
#ifndef SIMPLEHTTP_HTTPMETHODS_H_
#define SIMPLEHTTP_HTTPMETHODS_H_

#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>

/*External Vars*/
extern volatile atomic_bool ev_interrupt_received; // holds whether SIGINT received

/*Type Definitions*/
typedef enum { OK_, CREATED_, BAD_REQUEST_, FORBIDDEN_, NOT_FOUND_, ISE_, NOT_IMP_ } StatusC;
typedef enum {PUT=0, GET, HEAD} HTTPMethod;

/*Defines*/
#define BUFF_SIZE 4096

// Struct that contains different fields to execute each HTTP method
typedef struct {
    int client_fd;
    char *buffer;
    size_t current_buffer_position;
} ClientData;


/*Function Declarations*/
/** @brief Does the put request for the client
*   @param file: The file the client wants to access
*   @param buffer: Holds the client message to put to the file
*   @param clientFD: The file descriptor for the client
*   @param currentPosBuf: Current position of buffer
*   @param bytesRead: The number of bytes read from read method
*   @param contentLength: Length of the content to put in file
*   @param ReqNum:  Number of request from client (used in log file of server)
*   @param logFD:  Server log file descriptor to show status
*   @return 0 for success -1 for failure
*/

int HTTPutRequest(char* file, char* buffer, int clientFD, long int* currentPosBuf,
    long int* bytesRead, long int contentLength, long int ReqNum, int logFD);

/** @brief Does the get request for the client
*   @param file: The file the client wants to access
*   @param clientFD: The file descriptor for the client
*   @param ReqNum:  Number of request from client (used in log file of server)
*   @param logFD:  Server log file descriptor to show status
*   @return 0 for success -1 for failure
*/
int HTTPGetRequest(char* file, int clientFD, long int ReqNum, int logFD);

/** @brief Does the head request for the client
*   @param file: The file the client wants to access
*   @param clientFD: The file descriptor for the client
*   @param ReqNum:  Number of request from client (used in log file of server)
*   @param logFD:  Server log file descriptor to show status
*   @return 0 for success -1 for failure
*/
int HTTPHeadRequest(char* file, int clientFD, long int ReqNum, int logFD);


/** @brief Sends the status of the client command to the client
*   @param clientFD: File descriptor for client
*   @param status: The Status of the commands from client
*   @return void
*/
void StatusPrint(int clientFD, StatusC status);

/** @brief prints to the log file of the server
*   @param reqNum: The number given to client request
*   @param logFD: The file descriptor of the server log file
*   @param file: The file name the request wanted to use
*   @param method:  The method of the client request
*   @return void
*/
void LogFilePrint(long int reqNum, int logFD, int statusCode, char *file, char *method);
#endif
