/**
* Created By Fabert Charles
* port binding function declarations
*/

#ifndef BIND_H
#define BIND_H
/*Included Libraries*/
#include <stdint.h>

/*Function Declarations*/
/** @brief Parses port number and binds and listens on it
*   @param port:  Port to bind
*   @return  greater than 0 on success
*            -1 if invalid port number
*            -2 if opening socket failed
*            -3 if setsockopt failed
*            -4 if binding socket failed
*            -5 if listening failed
*/
int CreateSocket( uint16_t port );

/** @brief Converts string equivalent of port to a number
*   @param port: string
*   @return  > 0 on success
*            =< 0 on failure
*/
uint16_t StrToPort( char *port );
#endif
