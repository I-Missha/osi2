#ifndef __FITOS_list_H__
#define __FITOS_list_H__

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>
typedef struct _ListNode {
    int val;
    struct _ListNode *next;
    char string[100];
    pthread_rwlock_t rwlock;
} lnode_t;

typedef struct _List {
    lnode_t *first;
    lnode_t *last;

    pthread_t lmonitor_tid;

    pthread_mutex_t count_mutex;
    int count_equal;
    int count_less;
    int count_more;
    int count;
    int max_count;
    int swap_count;
    // pthread_cond_t get_cond;
    // pthread_cond_t add_cond;
    //  list statistics
} list_t;

list_t *list_init(int max_count);
void swap_nodes(lnode_t *first, lnode_t *second, lnode_t *parent);
void list_destroy(list_t *l);
int list_add_last(list_t *l, char *str);
int list_get(list_t *l, int *val);
void list_print_stats(list_t *l);

#endif // __FITOS_list_H__
