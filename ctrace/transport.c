#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "transport.h"


int
transport_open(const char *path)
{
    int sockfd;
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        return -1;

    struct sockaddr_un addr;
    if (strlen(path) >= sizeof(addr.sun_path)) {
        errno = EINVAL;
        return -1;
    }
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path);

    int e = connect(
        sockfd,
        (struct sockaddr *) &addr,
        sizeof(struct sockaddr_un)
    );
    if (e == -1)
        return -1;

    return sockfd;
}


int
transport_close(int sockfd)
{
    shutdown(sockfd, SHUT_RDWR);
    return close(sockfd);
}
