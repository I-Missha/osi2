#include "main.h"
#include "http_parser.h"

#define BUFFER_SIZE 4096 * 100

void *client_handler(void *arg) {
    printf("something happening\n");
    int client_fd = *(int *)arg;
    printf("something happening\n");
    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));

    printf("something happening\n");
    int recieve_size = recv(client_fd, (void *)buffer, BUFFER_SIZE, 0);
    printf("something happening\n");
    parse_http_request(buffer, recieve_size);
    return NULL;
}
#define CONNECTIONS_LIMIT 1000
int main() {
    int server_fd = create_server_fd(PROXY_PORT);
    int *client_connections = (int *)malloc(CONNECTIONS_LIMIT * sizeof(int));
    if (server_fd == -1) {
        perror(strerror(errno));
        return SOCKET_ERROR;
    }

    int connections_counter = 0;
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);
        int client_fd = accept(
            server_fd, (struct sockaddr *)&client_addr, &client_addr_size
        );

        if (client_fd == -1) {
            perror("accept failed\n");
            perror(strerror(errno));
            continue;
        }
        printf("new client connected %d\n", client_fd);
        pthread_t thread_id;
        client_connections[connections_counter] = client_fd;
        pthread_create(
            &thread_id,
            NULL,
            client_handler,
            &client_connections[connections_counter]
        );
        pthread_detach(thread_id);
        connections_counter++;
    }
    return 0;
}
