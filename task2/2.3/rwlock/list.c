#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <pthread.h>

#include "list.h"

void *lmonitor(void *arg) {
    list_t *l = (list_t *)arg;

    printf("lmonitor: [%d %d %d]\n", getpid(), getppid(), gettid());

    while (1) {
        list_print_stats(l);
        sleep(1);
    }

    return NULL;
}

list_t *list_init(int max_count) {
    int err;
    list_t *l = malloc(sizeof(list_t));
    if (!l) {
        printf("Cannot allocate memory for a list\n");
        abort();
    }
    l->first = NULL;
    l->last = NULL;
    l->count_equal = 0;
    l->count_less = 0;
    l->count_more = 0;
    l->max_count = max_count;
    l->count = 0;
    pthread_mutex_init(&l->count_mutex, NULL);
    err = pthread_create(&l->lmonitor_tid, NULL, lmonitor, l);
    if (err) {
        printf("list_init: pthread_create() failed: %s\n", strerror(err));
        abort();
    }

    return l;
}

int list_get(list_t *q, int *val) {

    assert(q->count >= 0);

    if (q->count == 0)
        return 0;

    lnode_t *tmp = q->first;

    *val = tmp->val;
    q->first = q->first->next;
    pthread_mutex_destroy(&tmp->mutex);
    free(tmp);

    return 1;
}

void list_destroy(list_t *l) {
    while (l->first) {
        int val = -1;
        list_get(l, &val);
        printf("stats from list destroy\n");
        list_print_stats(l);
    }
    free(l);
}

int list_add_last(list_t *l, char *str) {

    assert(l->count <= l->max_count);

    if (l->count == l->max_count)
        return 0;

    lnode_t *new = malloc(sizeof(lnode_t));
    if (!new) {
        printf("Cannot allocate memory for new node\n");
        abort();
    }
    pthread_mutex_init(&new->mutex, NULL);
    strcpy(new->string, str);
    new->next = NULL;
    if (!l->first)
        l->first = l->last = new;
    else {
        l->last->next = new;
        l->last = l->last->next;
    }

    l->count++;

    return 1;
}

// min size of list should be 4
// do not use swap_nodes on the last element, due to null in the second node
void swap_nodes(lnode_t *first, lnode_t *second, lnode_t *parent) {
    /*if (pthread_mutex_trylock(&parent->mutex) == EBUSY) {*/
    /*    return;*/
    /*}*/
    /*if (pthread_mutex_trylock(&first->mutex) == EBUSY) {*/
    /*    pthread_mutex_unlock(&parent->mutex);*/
    /*    return;*/
    /*}*/
    /*if (pthread_mutex_trylock(&second->mutex) == EBUSY) {*/
    /*    pthread_mutex_unlock(&parent->mutex);*/
    /*    pthread_mutex_unlock(&first->mutex);*/
    /*    return;*/
    /*}*/
    parent->next = second;
    first->next = second->next;
    second->next = first;
    /*pthread_mutex_unlock(&parent->mutex);*/
    /*pthread_mutex_unlock(&second->mutex);*/
    /*pthread_mutex_unlock(&first->mutex);*/
}

int list_get_first(list_t *l, int *val) {

    assert(l->count >= 0);

    if (l->count == 0)
        return 0;

    lnode_t *tmp = l->first;

    *val = tmp->val;
    l->first = l->first->next;

    free(tmp);
    l->count--;

    return 1;
}

void list_print_stats(list_t *l) {
    printf(
        "list stats: current size %d; counts (%d %d %d %d)\n",
        l->max_count,
        l->swap_count,
        l->count_more,
        l->count_less,
        l->count_equal
    );
}
