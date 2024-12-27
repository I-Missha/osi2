#ifndef __CACHE__LIB__
#define __CACHE__LIB__
#include <hashmap.h>
#include <pthread.h>
#include <string.h>
#include <vec.h>

#define MAX_CACHE_ENTRY_CAPCITY 4096 * 40
#define MAX_CACHE_CAPCITY MAX_CACHE_ENTRY_CAPCITY * 40
#define MAX_CAPACITY 4096 * 100

typedef struct CacheEntry {
    // url and content is a pointer to the vector var
    char **url;
    char **content;
    int curr_size;
    int is_full_content;
    uint8_t is_corresponds_to_cache_size;

    pthread_rwlock_t lock;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
} Entry;

typedef struct KeyValuePair {
    char **url;
    Entry *entry;
} Pair_t;

typedef struct Cache {
    struct hashmap *cache;
    int curr_size;

    pthread_cond_t cond;
    pthread_mutex_t mutex;
} Cache;
Pair_t *create_pair(char **key);
int my_compare(const void *a, const void *b, void *update);
uint64_t my_hash(const void *item, uint64_t seed0, uint64_t seed1);
Cache *create_cache();
#endif
