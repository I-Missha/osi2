#include "main.h"

#define BUFFER_SIZE 4096 * 100

void *client_handler(void *args) {
    int client_fd = *(int *)args;
    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));

    int recieve_size = recv(client_fd, (void *)buffer, BUFFER_SIZE, 0);
}

int main() {
    int server_fd = create_server_fd(PROXY_PORT);
    if (server_fd == -1) {
        perror(strerror(errno));
        return SOCKET_ERROR;
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);
        int client_fd = accept(
            server_fd, (struct sockaddr *)&client_addr, &client_addr_size
        );

        if (client_fd == -1) {
            perror("accept failed\n");
            perror(strerror(errno));
        }

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, NULL, (void *)&client_fd);
        pthread_detach(thread_id);
    }
    return 0;
}
