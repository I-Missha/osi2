#include "custom_socket.h"
#include <errno.h>
#include <error.h>
#include <string.h>

int create_server_fd(const int PORT) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("server socket failed\n");
        return ERROR;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    int reuse = 1;
    int result = setsockopt(
        server_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse)
    );

    if (result != 0) {
        perror("server setsockopt failed\n");
        return ERROR;
    }
    int error =
        bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (error == -1) {
        perror("server bind socket failed\n");
        return ERROR;
    }

    if (listen(server_fd, PENDING_NUMB) == -1) {
        perror("server set server socket to listen failed\n");
        return ERROR;
    }

    return server_fd;
}

int connect_via_host_name(const char *host_name) {
    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    struct addrinfo *addr_arr;

    int err = getaddrinfo(host_name, "http", &hints, &addr_arr);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        perror("can not resolve host name");
        freeaddrinfo(addr_arr);
        return 1;
    }

    struct addrinfo *iter = addr_arr;
    int host_fd;
    while (iter != NULL) {
        host_fd = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol);

        if (host_fd < 0) {
            /*printf(*/
            /*    "here %d\n%d\n%d\n",*/
            /*    iter->ai_family,*/
            /*    iter->ai_socktype,*/
            /*    iter->ai_protocol*/
            /*);*/
            /**/
            printf("here\n");
            iter = iter->ai_next;
            continue;
        }

        if (connect(host_fd, iter->ai_addr, iter->ai_addrlen) != -1) {
            break;
        }

        iter = iter->ai_next;
    }
    return host_fd;
}
