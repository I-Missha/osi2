#ifndef __PROXY__
#define __PROXY__

#include "cache.h"
#include "custom_socket.h"
#include "garbage_collector.h"
#include "http_parser.h"
#include <errno.h>
#include <llhttp.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vec.h>

#define PROXY_PORT 5040
#define CONNECTIONS_LIMIT 5040

#define SOCKET_ERROR 1

#define p_resS_LIMIT 1000
#endif
