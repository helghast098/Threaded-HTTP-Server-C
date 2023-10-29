/**
* Created By Fabert Charles
*
* Houses function declaration for httpserver
*/

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

/*Included Libraries*/
#include <stdint.h>

/*Function Declarations*/
/** @brief Checks if port given is valid
*   @param port:  Port num as 
*   @return port as a uint16_t
*/
uint16_t portCheck(char *port);

/** @brief Pieces the requestof client when the client is slow and determines if at the end of request
*   @param buffer:  Where the request of the client is placed (string)
*   @param bytesRead:  How many bytes read from read()
*   @param requestLine: The data gotten from read(clientFD)
*   @param currentReqPos: Current position in buffer
*   @param endRequest: True if end request, false otherwise
*   @param buffPosition:  Position in buffer
*   @param prevR_1:  True if encounter first \r
*   @param prevN_1:  True if after prevR_1 encounter \n
*   @param prevR_2:  True if after prevN_1 encounter \r
*   @return void
*/
void appropPlacer(char *buffer, long int bytesRead, char *requestLine, int *currentReqPos,
    bool *endRequest, long int *buffPosition, bool *prevR_1, bool *prevN_1, bool *prevR_2);
    
#endif