#include "cache.h"
#include <pthread.h>

/*int insert_entry() {}*/
int my_compare(const void *a, const void *b, void *udate) {
    const Pair_t *ma = (Pair_t *)a;
    const Pair_t *mb = (Pair_t *)b;
    return strcmp(ma->url, mb->url);
}

uint64_t my_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const Pair_t *pair = (Pair_t *)item;
    return hashmap_sip(pair->url, strlen(pair->url), seed0, seed1);
}

Entry *create_entry(char *url) {
    Entry *entry = (Entry *)malloc(sizeof(Entry));
    // tricky moment with url due to address of var
    entry->url = vector_copy(url);
    entry->content = vector_create();
    entry->curr_size = 0;
    entry->is_full_content = 0;
    pthread_mutex_init(&entry->mutex, NULL);
    pthread_cond_init(&entry->cond, NULL);
    pthread_rwlock_init(&entry->lock, NULL);
    return entry;
}

void destroy_entry(Entry *entry) {
    free(entry->url);
    free(entry->content);
    pthread_mutex_destroy(&entry->mutex);
    pthread_cond_destroy(&entry->cond);
    pthread_rwlock_destroy(&entry->lock);
    free(entry);
}

Pair_t *create_pair(char *key) {
    Pair_t *pair = (Pair_t *)malloc(sizeof(Pair_t));

    pair->url = key;
    pair->entry = create_entry(key);
    return pair;
}

void destroy_pair(Pair_t *pair) {
    destroy_entry(pair->entry);
    free(pair->url);
    free(pair);
}

Cache *create_cache() {
    Cache *cache = (Cache *)malloc(sizeof(Cache));
    cache->cache =
        hashmap_new(sizeof(Pair_t), 0, 0, 0, my_hash, my_compare, NULL, NULL);
    cache->curr_size = 0;
    return cache;
}
