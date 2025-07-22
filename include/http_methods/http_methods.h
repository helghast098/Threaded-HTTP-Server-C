/**
* Created by Fabert Charles
*
* Houses all functions for Http methods processing
*
*/
#ifndef SIMPLEHTTP_HTTPMETHODS_H_
#define SIMPLEHTTP_HTTPMETHODS_H_

#include "request_parser/request_parser.h" // for the Http Methods
#include "file_lock/file_lock.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>

/*External Vars*/

/*Type Definitions*/
typedef enum { OK_, CREATED_, BAD_REQUEST_, FORBIDDEN_, NOT_FOUND_, ISE_, NOT_IMP_ } StatusC;

/*Defines*/


/*Function Declarations*/
/** @brief Does the put request for the client
*   @param request: info of the request
*   @param client_buffer: client buffer which client_fd writes to
*   @param client_fd: The file descriptor for the client
*   @param log_fd: server log file descriptor
*   @param interrupt_received: Checks if server received shutdown command
*   @param file_locks: file locks to the files used in server
*   @return 0 for success -1 for failure
*/


int ExecuteRequest( Request *request, Buffer *client_buffer, int client_fd, int log_fd, atomic_bool *interrupt_received, FileLocks *file_locks  );

void LogFilePrint(long int reqNum, int logFD, int statusCode, char *file, char *method);

/** @brief Sends the status of the client command to the client
*   @param clientFD: File descriptor for client
*   @param status: The Status of the commands from client
*   @return void
*/
void StatusPrint(int clientFD, StatusC status);

#endif
