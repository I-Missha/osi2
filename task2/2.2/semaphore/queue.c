#define _GNU_SOURCE
#include <assert.h>

#include "queue.h"

void *qmonitor(void *arg) {
    queue_t *q = (queue_t *)arg;

    printf("qmonitor: [%d %d %d]\n", getpid(), getppid(), gettid());

    while (1) {
        queue_print_stats(q);
        sleep(1);
    }

    return NULL;
}

queue_t *queue_init(int max_count) {
    int err;
    queue_t *q = malloc(sizeof(queue_t));
    if (!q) {
        printf("Cannot allocate memory for a queue\n");
        abort();
    }
    sem_init(&q->sem, 0, 1);
    q->first = NULL;
    q->last = NULL;
    q->max_count = max_count;
    q->count = 0;

    q->add_attempts = q->get_attempts = 0;
    q->add_count = q->get_count = 0;

    err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
    if (err) {
        printf("queue_init: pthread_create() failed: %s\n", strerror(err));
        abort();
    }

    return q;
}

void queue_destroy(queue_t *q) {
    while (q->first) {
        int val = -1;
        queue_get(q, &val);
        printf("stats from queue destroy\n");
        queue_print_stats(q);
    }
    free(q);
}

int queue_add(queue_t *q, int val) {
    q->add_attempts++;

    assert(q->count <= q->max_count);

    sem_wait(&q->sem);
    if (q->count == q->max_count) {
        sem_post(&q->sem);
        return 0;
    }

    qnode_t *new = malloc(sizeof(qnode_t));
    if (!new) {
        printf("Cannot allocate memory for new node\n");
        sem_post(&q->sem);
        abort();
    }

    new->val = val;
    new->next = NULL;

    if (!q->first)
        q->first = q->last = new;
    else {
        q->last->next = new;
        q->last = q->last->next;
    }

    q->count++;
    q->add_count++;

    sem_post(&q->sem);
    return 1;
}

int queue_get(queue_t *q, int *val) {
    q->get_attempts++;

    assert(q->count >= 0);

    sem_wait(&q->sem);
    if (q->count == 0) {
        sem_post(&q->sem);
        return 0;
    }

    qnode_t *tmp = q->first;

    *val = tmp->val;
    q->first = q->first->next;

    free(tmp);
    q->count--;
    q->get_count++;

    sem_post(&q->sem);
    return 1;
}

void queue_print_stats(queue_t *q) {
    printf(
        "queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld "
        "%ld %ld)\n",
        q->count,
        q->add_attempts,
        q->get_attempts,
        q->add_attempts - q->get_attempts,
        q->add_count,
        q->get_count,
        q->add_count - q->get_count
    );
}
