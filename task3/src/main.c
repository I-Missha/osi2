#include "main.h"
#include "http_parser.h"

#define BUFFER_SIZE 4096 * 100
#define CONNECTIONS_LIMIT 1000

int is_acceptable_method(char *method) {}

void *client_handler(void *arg) {
    int client_fd = *(int *)arg;
    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));

    int recieve_size = recv(client_fd, (void *)buffer, BUFFER_SIZE, 0);
    llhttp_t parser;
    connection_request con_req;
    init_connection_request_structures(&parser, &con_req);
    parse_http_request(&parser, buffer, recieve_size);
    for (int i = 0; i < con_req.url_curr_size; i++) {
        printf("%c", con_req.url[i]);
    }
    printf("%d", llhttp_get_method(&parser));
    return NULL;
}

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
