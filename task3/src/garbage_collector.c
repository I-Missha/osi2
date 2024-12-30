#include "garbage_collector.h"

#define TIME_TO_CLEAR 60 * 10

void iterate_over_map(Cache *cache) {
    size_t iter = 0;
    void *item = NULL;
    while (hashmap_iter(cache->cache, &iter, &item)) {
        Pair_t *pair = item;
        pthread_mutex_lock(&pair->entry->mutex);

        if (pair->entry->is_realeased_by_gb) {
            pthread_mutex_unlock(&pair->entry->mutex);
            continue;
        }

        pair->entry->ref_counter--;
        pair->entry->is_realeased_by_gb = 1;

        if (pair->entry->ref_counter == 0) {
            hashmap_delete(cache->cache, pair);
            pthread_mutex_unlock(&pair->entry->mutex);

            destroy_pair(pair);

            continue;
        }
        pthread_mutex_unlock(&pair->entry->mutex);
    }
}

void *garbage_collector(void *arg) {
    GarbageCollectorArgs *args = arg;
    Cache *cache = args->cache;
    while (1) {
        struct timeval now;
        struct timespec timeout;
        // pizdec, mutex == wait
        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec + TIME_TO_CLEAR;
        timeout.tv_nsec = now.tv_usec * 1000;

        pthread_mutex_lock(&args->mutex);
        pthread_cond_timedwait(&args->cond, &args->mutex, &timeout);
        pthread_mutex_unlock(&args->mutex);

        pthread_rwlock_wrlock(&cache->lock);
        iterate_over_map(cache);
        pthread_rwlock_unlock(&cache->lock);
    }
    return NULL;
}
