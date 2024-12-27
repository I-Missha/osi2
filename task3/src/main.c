#include "main.h"
#include "cache.h"

typedef struct ServerArgs {
    int server_fd;
    ParseState *state;
    Pair_t *pair;
    Cache *cache;
} ServerArgs;

void *server_handler(void *arg) {
    ServerArgs *args = (ServerArgs *)arg;
    Pair_t *pair = args->pair;
    Cache *cache = args->cache;

    pthread_mutex_t *emutex = &pair->entry->mutex;
    pthread_cond_t *econd = &pair->entry->cond;
    pthread_mutex_t *cmutex = &cache->mutex;
    pthread_cond_t *ccond = &cache->cond;

    llhttp_t parser;
    Parser_res p_res;
    init_parser(&parser, &p_res, HTTP_RESPONSE, pair->entry->content, NULL);
    /*printf("%p\n", p_res.full_msg);*/
    /*printf("%p\n", pair->entry->content);*/
    int server_fd = args->server_fd;

    char *buff = (char *)malloc(BUFFER_SIZE);

    int rec_counter = 0;
    while (p_res.parseStateMsg != MsgParsed) {
        int rec_size = read(server_fd, buff, BUFFER_SIZE);
        pthread_mutex_lock(cmutex);
        while (cache->curr_size + rec_size > MAX_CACHE_CAPCITY) {
            pthread_cond_wait(ccond, cmutex);
        }

        // add cond signal for garbage collector

        pthread_mutex_lock(emutex);

        if (rec_counter + rec_size >= MAX_CACHE_ENTRY_CAPCITY) {
            pair->entry->is_corresponds_to_cache_size = 0;
            pthread_mutex_unlock(emutex);
            break;
        }

        char **copy = pair->entry->content;
        for (int i = 0; i < rec_size; i++) {
            vector_add(copy, buff[i]);
        }

        if (parse_http(&parser, buff, rec_size)) {
            pthread_mutex_unlock(emutex);
            return NULL;
        }
        cache->curr_size += rec_size;
        pthread_cond_signal(econd);
        pthread_mutex_unlock(emutex);
        pthread_mutex_unlock(cmutex);
    }

    pair->entry->is_full_content = 1;
    pthread_cond_signal(econd);
    // no for ordinary destroy
    // destroy_parser(&parser, &p_res);
    return NULL;
}

typedef struct ClientArgs {
    int client_fd;
    Cache *cache;
} ClientArgs;

int send_cached_content(const Pair_t *pair, int client_fd) {

    pthread_mutex_t *emutex = &pair->entry->mutex;
    pthread_cond_t *econd = &pair->entry->cond;
    char **content = pair->entry->content;
    vec_size_t written_counter = 0;
    while (1) {
        pthread_mutex_lock(emutex);
        vec_size_t to_write = vector_size(*content);
        while (written_counter == to_write && !pair->entry->is_full_content) {
            pthread_cond_wait(econd, emutex);
            to_write = vector_size(*content);
        }

        if (written_counter == to_write && pair->entry->is_full_content) {
            pthread_mutex_unlock(emutex);
            break;
        }

        int written = write(
            client_fd, *content + written_counter, to_write - written_counter
        );

        if (written < 0) {
            printf("%s\n", strerror(errno));
            pthread_mutex_unlock(emutex);
            return -1;
        }

        written_counter += written;
        pthread_mutex_unlock(emutex);
    }
    return 0;
}

void *client_handler(void *arg) {
    ClientArgs *client_args = (ClientArgs *)arg;
    int client_fd = client_args->client_fd;
    Cache *cache = client_args->cache;

    llhttp_t parser;
    Parser_res p_res;

    init_parser(&parser, &p_res, HTTP_REQUEST, NULL, NULL);

    if (receive_parsed_request(client_fd, &parser, &p_res)) {
        /*destroy_parser(&parser, &p_res);*/
        close(client_fd);
        free(client_args);
        return NULL;
    }

    const Pair_t *pair_cached =
        hashmap_get(cache->cache, &(Pair_t){.url = p_res.url});
    if (pair_cached) {
        send_cached_content(pair_cached, client_fd);
        close(client_fd);
        free(client_args);
        return NULL;
    }

    char *host_name = vector_create();
    http_parse_host_name(*p_res.url, &host_name);

    int server_fd = connect_via_host_name(host_name);
    if (server_fd < 0) {
        /*destroy_parser(&parser, &p_res);*/
        close(client_fd);
        free(client_args);
        return NULL;
    }
    /*printf("%s\n", p_res.full_msg);*/
    vec_size_t written_counter = 0;
    vec_size_t to_write = vector_size(*p_res.full_msg);
    while (written_counter != to_write) {
        int written = write(
            server_fd,
            *p_res.full_msg + written_counter,
            to_write - written_counter
        );

        if (written < 0) {
            /*destroy_parser(&parser, &p_res);*/
            close(client_fd);
            close(server_fd);
            free(client_args);
            return NULL;
        }

        written_counter += written;
    }

    pthread_t thread_id;
    ParseState server_parse_state = NotParsed;
    Pair_t *pair = create_pair(p_res.url);
    hashmap_set(cache->cache, pair);
    ServerArgs args = {server_fd, &server_parse_state, pair, cache};
    pthread_create(&thread_id, NULL, server_handler, &args);
    pthread_detach(thread_id);

    send_cached_content(pair, client_fd);
    /*destroy_request_parser(&parser, &p_res);*/
    close(client_fd);
    close(server_fd);
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
    }
    return 0;
}
