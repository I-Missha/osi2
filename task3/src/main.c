#include "main.h"
#include "cache.h"
#include "http_parser.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>

typedef struct ServerArgs {
    int server_fd;
    ParseState *state;
    Pair_t *pair;
    Cache *cache;
} ServerArgs;

int add_content_to_cache(Pair_t *pair) {
    char *content = pair->entry->content;
}

void *server_handler(void *arg) {
    ServerArgs *args = (ServerArgs *)arg;
    Pair_t *pair = args->pair;
    Cache *cache = args->cache;

    pthread_mutex_t emutex = pair->entry->mutex;
    pthread_cond_t econd = pair->entry->cond;
    pthread_mutex_t cmutex = cache->mutex;
    pthread_cond_t ccond = cache->cond;

    llhttp_t parser;
    Parser_res p_res;
    init_parser(&parser, &p_res, HTTP_RESPONSE, pair->entry->content, NULL);
    int server_fd = args->server_fd;

    const ParseState state = MsgParsed;
    char *buff = malloc(BUFFER_SIZE);

    int rec_counter = 0;
    while (p_res.parseStateMsg != state) {
        int rec_size = read(server_fd, buff, BUFFER_SIZE);
        pthread_mutex_lock(&cmutex);
        while (cache->curr_size + rec_size > MAX_CACHE_CAPCITY) {
            pthread_cond_wait(&ccond, &cmutex);
        }

        cache->curr_size += rec_size;
        // add cond signal for garbage collector
        pthread_mutex_unlock(&cmutex);

        pthread_mutex_lock(&emutex);
        if (rec_counter + rec_size >= MAX_CACHE_ENTRY_CAPCITY) {
            pair->entry->is_corresponds_to_cache_size = 0;
            pthread_mutex_unlock(&emutex);
            break;
        }
        // if size of msg less then capcity, add to cache
        vector_push_str(&p_res.full_msg, buff, rec_size);
        pthread_mutex_unlock(&emutex);
        if (parse_http(&parser, buff, rec_size)) {
            return NULL;
        }
    }
    pair->entry->is_full_content = 1;
    destroy_parser(&parser, &p_res);
    return NULL;
}

typedef struct ClientArgs {
    int client_fd;
    Cache *cache;
} ClientArgs;

void *client_handler(void *arg) {
    ClientArgs *client_args = (ClientArgs *)arg;
    int client_fd = client_args->client_fd;
    Cache *cache = client_args->cache;

    llhttp_t parser;
    Parser_res p_res;

    init_parser(&parser, &p_res, HTTP_REQUEST, NULL, NULL);

    if (receive_parsed_request(client_fd, &parser, &p_res)) {
        destroy_parser(&parser, &p_res);
        close(client_fd);
        free(client_args);
        return NULL;
    }

    char *host_name = vector_create();
    http_parse_host_name(p_res.url, &host_name);

    int host_fd = connect_via_host_name(host_name);
    if (host_fd < 0) {
        destroy_parser(&parser, &p_res);
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
            destroy_parser(&parser, &p_res);
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
    hashmap_set(cache->cache, pair);
    ServerArgs args = {host_fd, &server_parse_state, pair, cache};
    pthread_create(&thread_id, NULL, server_handler, &args);
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

    Cache *cache = create_cache();
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
