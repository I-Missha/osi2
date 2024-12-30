#include "main.h"
#include "cache.h"
#include "vec.h"
#include <pthread.h>

typedef struct ServerArgs {
    int server_fd;
    ParseState *state;
    Pair_t *pair;
    Cache *cache;
    pthread_cond_t *gar_cond;
} ServerArgs;

void *server_handler(void *arg) {
    ServerArgs *args = (ServerArgs *)arg;
    Pair_t *pair = args->pair;

    pthread_mutex_t *emutex = &pair->entry->mutex;

    llhttp_t parser;
    Parser_res p_res;
    init_parser(&parser, &p_res, HTTP_RESPONSE, pair->entry->content, NULL);
    int server_fd = args->server_fd;

    char *buff = (char *)malloc(BUFFER_SIZE);

    pthread_mutex_lock(emutex);
    pair->entry->ref_counter++;
    pthread_mutex_unlock(emutex);

    int rec_counter = 0;
    while (p_res.parseStateMsg != MsgParsed) {
        int rec_size = read(server_fd, buff, BUFFER_SIZE);
        if (rec_size <= 0) {
            pthread_mutex_lock(emutex);
            pair->entry->ref_counter--;
            pthread_cond_broadcast(&pair->entry->cond);
            pthread_mutex_unlock(emutex);

            close(server_fd);
            perror("cant read");
            return NULL;
        }

        if (rec_counter + rec_size >= MAX_CACHE_ENTRY_CAPCITY) {
            pthread_mutex_lock(emutex);
            pair->entry->is_corresponds_to_cache_size = 0;
            pthread_mutex_unlock(emutex);
            break;
        }

        char **copy = pair->entry->content;
        for (int i = 0; i < rec_size; i++) {
            vector_add(copy, buff[i]);
        }

        if (parse_http(&parser, buff, rec_size)) {
            return NULL;
        }

        pthread_cond_broadcast(&pair->entry->cond);
    }

    pthread_mutex_lock(emutex);
    pair->entry->is_full_content = 1;
    pair->entry->ref_counter--;
    pthread_mutex_unlock(emutex);

    return NULL;
}

typedef struct ClientArgs {
    int client_fd;
    Cache *cache;
    pthread_cond_t *gar_cond;
} ClientArgs;

int send_cached_content(Cache *cache, const Pair_t *pair, int client_fd) {

    if (!pair->entry->is_corresponds_to_cache_size) {
        return 0;
    }

    pthread_mutex_t *emutex = &pair->entry->mutex;
    char **content = pair->entry->content;
    vec_size_t written_counter = 0;

    pthread_mutex_lock(emutex);
    pair->entry->ref_counter++;
    pthread_mutex_unlock(emutex);

    while (1) {
        vec_size_t to_write = vector_size(*content);
        pthread_mutex_lock(emutex);
        while (written_counter == to_write && !pair->entry->is_full_content) {
            pthread_cond_wait(&pair->entry->cond, emutex);
            if (!pair->entry->is_corresponds_to_cache_size) {
                return 0;
            }
            to_write = vector_size(*content);
        }
        pthread_mutex_unlock(emutex);

        if (written_counter == to_write && pair->entry->is_full_content) {
            break;
        }

        int written = write(
            client_fd, *content + written_counter, to_write - written_counter
        );

        if (written < 0) {
            printf("%s\n", strerror(errno));
            break;
            /*return -1;*/
        }

        written_counter += written;
    }

    pthread_mutex_lock(emutex);
    pair->entry->ref_counter--;

    if (pair->entry->ref_counter == 0) {
        hashmap_delete(cache->cache, pair);
        pthread_mutex_unlock(emutex);
        destroy_pair(pair);

        return 0;
    }
    pthread_mutex_unlock(emutex);
    return 0;
}

Pair_t *hashmap_get_or_set(Cache *cache, char **url) {
    Pair_t *temp = create_pair_url_only(url);
    pthread_rwlock_wrlock(&cache->lock);
    Pair_t *pair_cached = hashmap_get(cache->cache, temp);
    vector_free(*temp->url);
    free(temp->url);
    free(temp);
    if (!pair_cached) {
        Pair_t *pair = create_pair(url);
        hashmap_set(cache->cache, pair);
        pthread_rwlock_unlock(&cache->lock);
        return NULL;
    }
    pthread_rwlock_unlock(&cache->lock);
    return pair_cached;
}

void *client_handler(void *arg) {
    ClientArgs *client_args = (ClientArgs *)arg;
    int client_fd = client_args->client_fd;
    Cache *cache = client_args->cache;

    llhttp_t parser;
    Parser_res p_res;

    init_parser(&parser, &p_res, HTTP_REQUEST, NULL, NULL);

    if (receive_parsed_request(client_fd, &parser, &p_res)) {
        close(client_fd);
        free(client_args);
        return NULL;
    }

    Pair_t *pair_cached = hashmap_get_or_set(cache, p_res.url);

    if (pair_cached) {
        send_cached_content(cache, pair_cached, client_fd);
        close(client_fd);
        free(client_args);
        return NULL;
    }

    char *host_name = vector_create();
    http_parse_host_name(*p_res.url, &host_name);

    int server_fd = connect_via_host_name(host_name);
    if (server_fd < 0) {
        close(client_fd);
        free(client_args);
        return NULL;
    }

    vec_size_t written_counter = 0;
    vec_size_t to_write = vector_size(*p_res.full_msg);
    while (written_counter != to_write) {
        int written = write(
            server_fd,
            *p_res.full_msg + written_counter,
            to_write - written_counter
        );

        if (written < 0) {
            close(client_fd);
            close(server_fd);
            free(client_args);
            return NULL;
        }

        written_counter += written;
    }

    pthread_t thread_id;
    ParseState server_parse_state = NotParsed;

    Pair_t *temp = create_pair_url_only(p_res.url);
    pthread_rwlock_rdlock(&cache->lock);
    Pair_t *pair = hashmap_get(cache->cache, temp);
    pthread_rwlock_unlock(&cache->lock);
    vector_free(*temp->url);
    free(temp->url);
    free(temp);

    ServerArgs args = {
        server_fd, &server_parse_state, pair, cache, client_args->gar_cond
    };

    pthread_create(&thread_id, NULL, server_handler, &args);
    pthread_detach(thread_id);

    send_cached_content(cache, pair, client_fd);
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
    signal(SIGPIPE, SIG_IGN);
    GarbageCollectorArgs *gar_args = malloc(sizeof(GarbageCollectorArgs));
    gar_args->cache = cache;
    gar_args->cache_cond = &cache->cond;
    pthread_mutex_init(&gar_args->mutex, NULL);
    pthread_cond_init(&gar_args->cond, NULL);
    pthread_t gar_id;
    pthread_create(&gar_id, NULL, garbage_collector, gar_args);
    pthread_detach(gar_id);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);
        int client_fd = accept(
            server_fd, (struct sockaddr *)&client_addr, &client_addr_size
        );

        if (client_fd == -1) {
            perror(strerror(errno));
            continue;
        }

        pthread_t thread_id;
        ClientArgs *client_args = (ClientArgs *)malloc(sizeof(ClientArgs));

        client_args->client_fd = client_fd;
        client_args->cache = cache;
        client_args->gar_cond = &gar_args->cond;
        pthread_create(&thread_id, NULL, client_handler, client_args);
        pthread_detach(thread_id);
    }
    return 0;
}
