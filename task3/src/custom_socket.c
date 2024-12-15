#include "custom_socket.h"

int create_server_fd(const int PORT) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("server socket failed");
        return ERROR;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    int error =
        bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (error == -1) {
        perror("server bind server socket failed");
        return ERROR;
    }

    if (listen(server_fd, PENDING_NUMB) == -1) {
        perror("server set server socket to listen failed");
        return ERROR;
    }

    return server_fd;
}
