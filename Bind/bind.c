/**
* Created By Fabert Charles
*
* Houses porting binding function definitions
*/

/*Included Libraries*/
#include "bind.h"
#include <signal.h>
#include <string.h>
#include <arpa/inet.h>

/*Function Definitions*/
int create_listen_socket(uint16_t port) {
    signal(SIGPIPE, SIG_IGN);
    if (port == 0) { 
        return -1;
    }

    struct sockaddr_in addr;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (listenfd < 0) {
        return -2;
    }

    int on = 1;

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        return -3;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(listenfd, (struct sockaddr *) &addr, sizeof(addr) < 0)) {
        return -4;
    }

    if (listen(listenfd, 500) < 0) {
        return -4;
    }
    return listenfd;
}
