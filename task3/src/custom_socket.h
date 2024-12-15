#ifndef __SOCKET__LIB__
#define __SOCKET__LIB__
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>

#define ERROR -1
#define PENDING_NUMB 20

int create_server_fd(const int PORT);
#endif
