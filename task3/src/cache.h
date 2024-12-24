#ifndef __CACHE__LIB__
#define __CACHE__LIB__
#include <hashmap.h>
#include <pthread.h>
#include <string.h>
#define MAX_CAPACITY 4096 * 100
#include <vec.h>
typedef struct CacheEntry {
    // url and content is vector
    char *url;
    char *content;
    int max_capacity;

    pthread_rwlock_t lock;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
} Entry;

typedef struct KeyValuePair {
    char *url;
    Entry *entry;
} Pair_t;

Pair_t *create_pair(char *key);
int my_compare(const void *a, const void *b, void *update);
uint64_t my_hash(const void *item, uint64_t seed0, uint64_t seed1);
struct hashmap *create_cache();
#endif
