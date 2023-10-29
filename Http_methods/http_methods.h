/**
* Created by Fabert Charles
*
* Houses all functions for Http methods processing
*
*/

#ifndef HTTP_METHODS_H
#define HTTP_METHODS_H

/*Libraries Included*/
#include<stdbool.h>
#include<stdatomic.h>

/*Variable declarations*/
extern volatile atomic_bool ev_signQuit; // Shared Extern vars for sign


/*Type Definitions*/
typedef enum { OK_, CREATED_, BAD_REQUEST_, FORBIDDEN_, NOT_FOUND_, ISE_, NOT_IMP_ } StatusC;

/*Defines*/
#define BUFF_SIZE 4096

/*Function Declarations*/
/** @brief Does the puth method for the client
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
int http_methods_PutReq(char* file, char* buffer, int clientFD, long int* currentPosBuf,
    long int* bytesRead, long int contentLength, long int ReqNum, int logFD);

/** @brief Does the get method for the client
*   @param file: The file the client wants to access
*   @param clientFD: The file descriptor for the client
*   @param ReqNum:  Number of request from client (used in log file of server)
*   @param logFD:  Server log file descriptor to show status
*   @return 0 for success -1 for failure
*/
int http_methods_GetReq(char* file, int clientFD, long int ReqNum, int logFD);

/** @brief Does the head method for the client
*   @param file: The file the client wants to access
*   @param clientFD: The file descriptor for the client
*   @param ReqNum:  Number of request from client (used in log file of server)
*   @param logFD:  Server log file descriptor to show status
*   @return 0 for success -1 for failure
*/
int http_methods_HeadReq(char* file, int clientFD, long int ReqNum, int logFD);


/** @brief Sends the status of the client command to the client
*   @param clientFD: File descriptor for client
*   @param status: The Status of the commands from client
*   @return void
*/
void http_methods_StatusPrint(int clientFD, StatusC status);
#endif
