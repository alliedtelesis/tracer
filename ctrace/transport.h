#ifndef _TRANSPORT_H
#define _TRANSPORT_H    1

int transport_open(const char *path);
int transport_close(int sockfd);

#endif /* transport.h */
