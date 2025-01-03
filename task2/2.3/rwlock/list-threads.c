#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "list.h"

#define RED "\033[41m"
#define NOCOLOR "\033[0m"

void set_cpu(int n) {
    int err;
    cpu_set_t cpuset;
    pthread_t tid = pthread_self();

    CPU_ZERO(&cpuset);
    CPU_SET(n, &cpuset);

    err = pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset);
    if (err) {
        printf("set_cpu: pthread_setaffinity failed for cpu %d\n", n);
        return;
    }

    printf("set_cpu: set cpu %d\n", n);
}

int isLess(lnode_t *first, lnode_t *second) {
    return strlen(first->string) < strlen(second->string);
}
int isMore(lnode_t *first, lnode_t *second) {
    return strlen(first->string) > strlen(second->string);
}
int isEqual(lnode_t *first, lnode_t *second) {
    return strlen(first->string) == strlen(second->string);
}
void addOne(int (*operation)(lnode_t *, lnode_t *), list_t *list) {
    if (operation == isMore) {
        __sync_fetch_and_add(&list->count_more, 1);
    } else if (operation == isLess) {
        __sync_fetch_and_add(&list->count_less, 1);
    } else {
        __sync_fetch_and_add(&list->count_equal, 1);
    }
}

// [list*, comp_func]
// ToDo: struct like this array
typedef struct thread_comparision_args {
    list_t *list;
    int (*operation)(lnode_t *, lnode_t *);
} thread_comparision_args;

void *thread_comparision(void *args) {
    thread_comparision_args *thread_args = (thread_comparision_args *)args;
    list_t *list = thread_args->list;
    int (*operation)(lnode_t *, lnode_t *) = thread_args->operation;
    lnode_t *iter = list->first;
    while (1) {
        pthread_rwlock_rdlock(&iter->rwlock);
        pthread_rwlock_t *rwlock = &iter->rwlock;
        if (iter->next == NULL) {
            pthread_rwlock_rdlock(&list->first->rwlock);
            iter = list->first;
            addOne(operation, list);
        } else {
            pthread_rwlock_rdlock(&iter->next->rwlock);
            operation(iter, iter->next);
            iter = iter->next;
        }
        pthread_rwlock_unlock(&iter->rwlock);
        pthread_rwlock_unlock(rwlock);
    }
    return 0;
}

#define RAND_SWAP 1
#define RAND_MODULE 20
void *swap_threads(void *args) {
    list_t *list = (list_t *)args;
    srand(time(NULL));
    lnode_t *iter = list->first;
    while (1) {
        int r = rand() % RAND_MODULE;
        if (r == RAND_SWAP && iter && iter->next && iter->next->next &&
            iter->next->next->next) {
            pthread_rwlock_wrlock(&iter->rwlock);
            pthread_rwlock_wrlock(&iter->next->rwlock);
            pthread_rwlock_wrlock(&iter->next->next->rwlock);
            swap_nodes(iter->next, iter->next->next, iter);
            pthread_rwlock_unlock(&iter->next->next->rwlock);
            pthread_rwlock_unlock(&iter->next->rwlock);
            pthread_rwlock_unlock(&iter->rwlock);
        }
        pthread_rwlock_rdlock(&iter->rwlock);
        pthread_rwlock_t *rwlock = &iter->rwlock;
        if (iter->next == NULL) {
            pthread_rwlock_rdlock(&list->first->rwlock);
            iter = list->first;
            __sync_fetch_and_add(&list->swap_count, 1);
        } else {
            pthread_rwlock_rdlock(&iter->next->rwlock);
            iter = iter->next;
        }
        pthread_rwlock_unlock(&iter->rwlock);
        pthread_rwlock_unlock(rwlock);
    }
    return 0;
}

void fill_list(list_t *list, int size) {
    int i = 0;
    char arr[100] = {1};
    while (i < size) {
        arr[i % 100] = '\0';
        int ok = list_add_last(list, arr);
        if (!ok)
            continue;
        arr[i % 100] = 1;
        i++;
    }
}

int main() {
    pthread_t tid;
    list_t *l;
    int err;

    printf("main [%d %d %d]\n", getpid(), getppid(), gettid());

    l = list_init(100000);
    fill_list(l, 100000);
    thread_comparision_args args_more = {l, isMore};
    err = pthread_create(&tid, NULL, thread_comparision, &args_more);
    thread_comparision_args args_less = {l, isLess};
    err = pthread_create(&tid, NULL, thread_comparision, &args_less);
    thread_comparision_args args_equal = {l, isEqual};
    err = pthread_create(&tid, NULL, thread_comparision, &args_equal);
    if (err) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }

    for (int i = 0; i < 3; i++) {
        err = pthread_create(&tid, NULL, swap_threads, l);
        if (err) {
            printf("main: pthread_create() failed: %s\n", strerror(err));
            return -1;
        }
    }
    sched_yield();

    pthread_exit(NULL);

    return 0;
}
