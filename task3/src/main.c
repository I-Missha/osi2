#include "main.h"
#include "cache.h"
#include "http_parser.h"
#include <bits/pthreadtypes.h>

typedef struct hashmap hashmap;

typedef struct ServerArgs {
    int server_fd;
    ParseState *state;
    Pair_t *pair;
    hashmap *cache;
} ServerArgs;

void *server_handler(void *arg) {
    ServerArgs *args = (ServerArgs *)arg;
    Pair_t *pair = args->pair;

    llhttp_t parser;
    Parser_res p_res;
    init_request_parser(&parser, &p_res, HTTP_RESPONSE);
    int server_fd = args->server_fd;

    // create cache entry

    char **server_ans = &pair->entry->content;
    const ParseState state = Parsed;
    char *buff = malloc(BUFFER_SIZE);

    // now i need to receive content
    while (p_res.parseState != state) {
        int rec_size = read(server_fd, buff, BUFFER_SIZE);

        vector_push_str(&p_res.full_msg, buff, rec_size);
        if (parse_http_request(&parser, buff, rec_size)) {
            return NULL;
        }
    }
    *args->isParsed = 1;
    destroy_request_parser(&parser, &p_res);
    return NULL;
}

typedef struct ClientArgs {
    int client_fd;
    hashmap *cache;
} ClientArgs;

void *client_handler(void *arg) {
    ClientArgs *client_args = (ClientArgs *)arg;
    int client_fd = client_args->client_fd;
    hashmap *cache = client_args->cache;

    llhttp_t parser;
    Parser_res p_res;
    init_request_parser(&parser, &p_res, HTTP_REQUEST);

    if (receive_parsed_request(client_fd, &parser, &p_res)) {
        destroy_request_parser(&parser, &p_res);
        close(client_fd);
        free(client_args);
        return NULL;
    }

    if (!is_method_acceptable(&p_res) || !is_version_acceptable(&p_res)) {
        destroy_request_parser(&parser, &p_res);
        close(client_fd);
        return NULL;
    }
    char *host_name = vector_create();
    http_parse_host_name(p_res.url, &host_name);

    int host_fd = connect_via_host_name(host_name);
    if (host_fd < 0) {
        destroy_request_parser(&parser, &p_res);
        close(client_fd);
        free(client_args);
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
            free(client_args);
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
    // writing inition of server args
    pthread_t thread_id;

    ParseState server_parse_state = NotParsed;
    Pair_t *pair = create_pair(p_res.url);
    ServerArgs args = {host_fd, &server_parse_state, pair, cache};
    /*pthread_create(&thread_id, NULL, server_handler, &args);*/
    /*pthread_join(thread_id, NULL);*/
    /**/
    /*printf("%s\n", server_ans);*/
    /*written_counter = 0;*/
    /*to_write = vector_size(server_ans);*/
    /*while (written_counter != to_write) {*/
    /*    int written = write(*/
    /*        client_fd, server_ans + written_counter, to_write -
     * written_counter*/
    /*    );*/
    /**/
    /*    if (written < 0) {*/
    /*        destroy_request_parser(&parser, &p_res);*/
    /*        close(client_fd);*/
    /*        close(host_fd);*/
    /*        return NULL;*/
    /*    }*/
    /**/
    /*    written_counter += written;*/
    /*}*/
    /*destroy_request_parser(&parser, &p_res);*/
    /*close(client_fd);*/
    /*close(host_fd);*/
    free(client_args);
    return NULL;
}

int main() {
    int server_fd = create_server_fd(PROXY_PORT);
    if (server_fd == -1) {
        perror(strerror(errno));
        return SOCKET_ERROR;
    }

    hashmap *cache = create_cache();
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
        ClientArgs *client_args = (ClientArgs *)malloc(sizeof(ClientArgs));
        client_args->client_fd = client_fd;
        // write inition of cache
        client_args->cache = cache;
        pthread_create(&thread_id, NULL, client_handler, client_args);
        pthread_detach(thread_id);
        sleep(10);
    }
    return 0;
}
