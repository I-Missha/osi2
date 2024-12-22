#include "main.h"

int is_method_acceptable(Parser_res *p_res) {
    llhttp_method_t method = p_res->method;
    return method == HTTP_GET;
}

int is_version_acceptable(Parser_res *p_res) {
    uint8_t minor_version = p_res->minor_version;
    uint8_t major_version = p_res->major_version;
    return (minor_version == 0 || minor_version == 1) && major_version == 1;
}

void vector_push_str(char **vec, char *str, int str_size) {
    for (int i = 0; i < str_size; i++) {
        vector_add(vec, str[i]);
    }
}

int receive_parsed_request(int client_fd, llhttp_t *parser, Parser_res *p_res) {
    const ParseState state = ReqParsed;
    char *buff = malloc(BUFFER_SIZE);
    while (p_res->parseState != state) {
        int rec_size = read(client_fd, buff, BUFFER_SIZE);
        vector_push_str(&p_res->full_msg, buff, rec_size);
        if (parse_http_request(parser, buff, rec_size)) {
            return PARSE_ERROR;
        }
    }
    return SUCCESS_PARSE;
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

    printf("%d\n", llhttp_get_method(&parser));
    if (!is_method_acceptable(&p_res) || !is_version_acceptable(&p_res)) {
        destroy_request_parser(&parser, &p_res);
        close(client_fd);
        return NULL;
    }

    for (int i = 0; i < vector_size(p_res.url); i++) {
        printf("%c", p_res.url[i]);
    }

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
