#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "transport.h"

/* Environment variable settings */
#define TRACE_TRANSPORT_HOST "TRACE_HOST"
#define TRACE_TRANSPORT_PORT "TRACE_PORT"
#define TRACE_TRANSPORT_UNIX "TRACE_UNIX"

/**
 * Trace transport with TCP/unix
 *
 * The path to the unix socket is provided by the environment variable
 * defined by TRACE_TRANSPORT_UNIX 
 *
 * Returns a socket file descriptor
 */
int trace_transport_unix(void)
{
    struct sockaddr_un addr;
    char *path;

    int sockfd;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        /* Socket says no */
        return TRACE_TRANSPORT_ERROR_SOCK;
    }

    /* Get path from environment */
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    if ((path = getenv(TRACE_TRANSPORT_UNIX)) == NULL) {
        /* Socket path was not set */
        return TRACE_TRANSPORT_ERROR_ADDR;
    }
    strcpy(addr.sun_path, path);

    if (connect(
            sockfd, (struct sockaddr *) &addr,
            sizeof(struct sockaddr_un)
        ) == -1) { 
        /* Connection failed */
        return TRACE_TRANSPORT_ERROR_CONN;
    }

    return sockfd;
}

/**
 * Trace transport with TCP/inet
 *
 * The host and port of the target are provided by the environment variables
 * defined by TRACE_TRANSPORT_HOST and TRACE_TRANSPORT_PORT
 *
 * Returns a socket file descriptor
 */
int trace_transport_inet(void)
{
    struct addrinfo hints;
    struct addrinfo *addr_list;
    struct addrinfo *addr;
    char *host;
    char *port;

    int sockfd;

    /* Fill out address hints */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* Support IPv4 and IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    /* Get host and port from environment */
    if ((host = getenv(TRACE_TRANSPORT_HOST)) == NULL) {
        /* No host set */
        return TRACE_TRANSPORT_ERROR_ADDR;
    }
    if ((port = getenv(TRACE_TRANSPORT_PORT)) == NULL) {
        /* No port set */
        return TRACE_TRANSPORT_ERROR_ADDR;
    }

    /* Lookup hostname */
    getaddrinfo(host, port, &hints, &addr_list);

    /* Loop through linked list of addrs until we find one that works */
    for (addr = addr_list; addr != NULL; addr = addr->ai_next) {
        sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sockfd == -1) {
            /* Socket says no */
            continue;
        }
        if (connect(sockfd, addr->ai_addr, addr->ai_addrlen) != -1) {
            /* Connected */
            break;
        }
        /* Couldnt connect */
        close(sockfd);
    }
    if (addr == NULL) {
        /* Couldnt connect */
        sockfd = TRACE_TRANSPORT_ERROR_CONN;
    }
    freeaddrinfo(addr_list);

    return sockfd;
}

