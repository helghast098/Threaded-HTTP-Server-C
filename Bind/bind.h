/**
* Created By Fabert Charles
* Houses port binding function declarations
*/

#ifndef BIND_H
#define BIND_H
/*Included Libraries*/
#include <stdint.h>

/*Function Declarations*/
/** @brief Parses port number and binds and listens on it
*   @param port:  Port to bind
*   @return fd > 0 on success
            -1 if invalid port number
            -2 if opening socket failed
            -3 if binding socket failed
            -4 if listening failed
*/
int create_listen_socket(uint16_t port);
#endif
