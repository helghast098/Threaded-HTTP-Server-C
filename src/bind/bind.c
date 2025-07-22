/**
 * Created By Fabert Charles
 *
 * Houses porting binding function definitions
 */

/*Included Libraries*/
#include "bind/bind.h"

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <arpa/inet.h>
#include <err.h>
#include <stdio.h>
/*Function Definitions*/



int  CreateSocket( uint16_t port ) {
    signal( SIGPIPE, SIG_IGN );
    if ( port == 0 ) {
        return -1;
    }
    struct sockaddr_in addr;
    int listenfd = socket( AF_INET, SOCK_STREAM, 0 );

    if ( listenfd < 0 ) {
        return -2;
    }
    int on = 1;

    if ( setsockopt( listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof( on ) ) < 0 ) {
        return -3;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if ( bind( listenfd, ( struct sockaddr * ) &addr, sizeof( addr ) ) < 0 ) {
        return -4;
    }

    if ( listen( listenfd, 500 ) < 0 ) {
        return -5;
    }
    return listenfd;
}

int32_t StrToPort( char *port ) {
    // Checking values of char
    for ( unsigned int i = 0; port[ i ] != '\0'; ++i ) {
        if ( port[ i ] < '0' || port[ i ] > '9' ) {
            warnx( "invalid port number: %s", port );
            return -1;
        }
    }

    // Turning string to long
    return strtol( port, NULL, 10 );
}