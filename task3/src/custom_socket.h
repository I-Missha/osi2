#ifndef __SOCKET__LIB__
#define __SOCKET__LIB__
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#define ERROR -1
#define PENDING_NUMB 20

int connect_via_host_name(const char *host_name);
int create_server_fd(const int PORT);
#endif
