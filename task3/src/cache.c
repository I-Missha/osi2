#include "cache.h"

int my_compare(const void *a, const void *b, void *udate) {
    const Pair_t *ma = (Pair_t *)a;
    const Pair_t *mb = (Pair_t *)b;
    return strcmp(*ma->url, *mb->url);
}

uint64_t my_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const Pair_t *pair = (Pair_t *)item;
    return hashmap_sip(*pair->url, vector_size(*pair->url), seed0, seed1);
}

static char **create_vector_handler() {
    char **vec_ptr = (char **)malloc(sizeof(char *));
    char *new_vec = vector_create();
    *vec_ptr = new_vec;
    return vec_ptr;
}

static char **create_vector_handler_from_str(char *str) {
    char **vec_ptr = (char **)malloc(sizeof(char *));
    char *just = vector_create();
    *vec_ptr = just;
    for (int i = 0; i < vector_size(str); i++) {
        vector_add(vec_ptr, str[i]);
    }

    return vec_ptr;
}

Entry *create_entry(char *url) {
    Entry *entry = (Entry *)malloc(sizeof(Entry));
    // tricky moment with url due to address of var
    entry->url = create_vector_handler();
    *entry->url = vector_copy(url);
    entry->content = create_vector_handler();
    entry->curr_size = 0;
    entry->is_full_content = 0;
    entry->is_corresponds_to_cache_size = 1;
    entry->ref_counter = 1;
    entry->is_realeased_by_gb = 0;
    pthread_mutex_init(&entry->mutex, NULL);
    return entry;
}

void destroy_entry(Entry *entry) {
    vector_free(*entry->content);
    free(entry->content);
    pthread_mutex_destroy(&entry->mutex);
    vector_free(*entry->url);
    free(entry->url);
    free(entry);
}

Pair_t *create_pair(char **key) {
    Pair_t *pair = (Pair_t *)malloc(sizeof(Pair_t));

    pair->url = create_vector_handler_from_str(*key);
    pair->entry = create_entry(*key);
    return pair;
}

void destroy_pair(const Pair_t *pair) {
    destroy_entry(pair->entry);
    vector_free(*pair->url);
    free(pair->url);
}

Cache *create_cache() {
    Cache *cache = (Cache *)malloc(sizeof(Cache));
    pthread_mutex_init(&cache->mutex, NULL);
    pthread_cond_init(&cache->cond, NULL);
    pthread_rwlock_init(&cache->lock, NULL);

    cache->cache =
        hashmap_new(sizeof(Pair_t), 0, 0, 0, my_hash, my_compare, NULL, NULL);
    cache->curr_size = 0;
    return cache;
}
