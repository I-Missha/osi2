#ifndef __PROXY__
#define __PROXY__

#include "custom_socket.h"
#include "http_parser.h"
#include "vec.h"
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define PROXY_PORT 5040
#define CONNECTIONS_LIMIT 5040

#define SOCKET_ERROR 1
#define PARSE_ERROR 2
#define SUCCESS_PARSE 0

#define BUFFER_SIZE 4096 * 100
#define p_resS_LIMIT 1000
#endif
