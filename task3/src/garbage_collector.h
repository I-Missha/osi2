#ifndef __GARBAGE_COLLECTOR__
#define __GARBAGE_COLLECTOR__
#include "cache.h"
#include "hashmap.h"
#include "vec.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

void *garbage_collector(void *arg);

typedef struct GarbageCollectorArgs {
    Cache *cache;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
} GarbageCollectorArgs;

#endif
