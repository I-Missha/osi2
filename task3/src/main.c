#include "main.h"
#include "custom_socket.h"
#include "vec.h"

int is_method_acceptable(Parser_res *p_res) {
    llhttp_method_t method = p_res->method;
    return method == HTTP_GET;
}

int is_version_acceptable(Parser_res *p_res) {
    uint8_t minor_version = p_res->minor_version;
    uint8_t major_version = p_res->major_version;
    return (minor_version == 0 || minor_version == 1) && major_version == 1;
}

void *client_handler(void *arg) {
    int client_fd = *(int *)arg;

    llhttp_t parser;
    Parser_res p_res;
    init_request_parser(&parser, &p_res);

    if (receive_parsed_request(client_fd, &parser, &p_res)) {
        destroy_request_parser(&parser, &p_res);
        close(client_fd);
        return NULL;
    }

    // || !is_version_acceptable(&p_res)
    if (!is_method_acceptable(&p_res)) {
        destroy_request_parser(&parser, &p_res);
        close(client_fd);
        return NULL;
    }
    char *host_name = vector_create();
    http_parse_host_name(p_res.url, &host_name);

    int host_fd = connect_via_host_name(host_name);
    if (host_fd > 0) {
        destroy_request_parser(&parser, &p_res);
        close(client_fd);
        return NULL;
    }

    vec_size_t written_counter = 0;
    vec_size_t to_write = vector_size(p_res.full_msg);
    while (written_counter != to_write) {
        int written = write(
            host_fd,
            p_res.full_msg + written_counter,
            to_write - written_counter
        );

        if (written < 0) {
            destroy_request_parser(&parser, &p_res);
            close(client_fd);
            close(host_fd);
            return NULL;
        }

        written_counter += written;
    }
    /*printf("%s\n", p_res.full_msg);*/
    /*for (int i = 0; i < vector_size(host_name); i++) {*/
    /*    printf("%c", host_name[i]);*/
    /*}*/
    /**/
    /*printf("\n");*/
    return NULL;
}

int main() {
    int server_fd = create_server_fd(PROXY_PORT);
    int *client_p_ress = (int *)malloc(CONNECTIONS_LIMIT * sizeof(int));
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
        client_p_ress[connections_counter] = client_fd;
        pthread_create(
            &thread_id,
            NULL,
            client_handler,
            &client_p_ress[connections_counter]
        );
        pthread_detach(thread_id);
        connections_counter++;
    }
    return 0;
}
