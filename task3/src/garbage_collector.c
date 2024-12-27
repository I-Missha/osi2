#include "garbage_collector.h"
#include "cache.h"

#define TIME_TO_CLEAR 10

void iterate_over_map(Cache *cache) {
    size_t iter = 0;
    void *item = NULL;
    while (hashmap_iter(cache->cache, &iter, &item)) {
        Pair_t *pair = item;
        pthread_mutex_lock(&pair->entry->mutex);

        if (pair->entry->time_counter != TIME_TO_CLEAR) {
            pair->entry->time_counter++;
            pthread_mutex_unlock(&pair->entry->mutex);
            continue;
        }

        cache->curr_size -= vector_size(*pair->entry->content);
        hashmap_delete(cache->cache, item);
        destroy_entry(pair->entry);
        pthread_mutex_unlock(&pair->entry->mutex);
    }
}

void *garbage_collector(void *arg) {
    GarbageCollectorArgs *args = arg;
    Cache *cache = args->cache;
    while (1) {
        struct timeval now;
        struct timespec timeout;
        int retcode;

        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec + 1;
        timeout.tv_nsec = now.tv_usec * 1000;
        retcode = 0;

        retcode = pthread_cond_timedwait(&args->cond, &args->mutex, &timeout);

        pthread_mutex_lock(&cache->mutex);

        if (retcode == ETIMEDOUT) {
            iterate_over_map(cache);
            pthread_mutex_unlock(&cache->mutex);
            continue;
        }

        while (cache->curr_size >= MAX_CACHE_CAPCITY - MAX_CACHE_ENTRY_CAPCITY
        ) {
            iterate_over_map(cache);
        }

        pthread_mutex_unlock(&cache->mutex);
    }
    return NULL;
}
