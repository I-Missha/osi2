#define _GNU_SOURCE
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/futex.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <ucontext.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define STACK_SIZE PAGE_SIZE * 4
#define NAME_SIZE 16
#define MAX_THREADS 8
#define MAX_STACKS_TO_CLEAR 100

typedef struct uthread {
    void *stack;
    char name[NAME_SIZE];
    void (*thread_func)(void *);
    void *arg;
    ucontext_t uctx;
    struct uthread *next;
} uthread_t;

typedef struct uthreads_list {
    uthread_t *first;
    uthread_t *last;
    uthread_t *curr;
} uthreads_list;

typedef struct stack_to_clear {
    void **stacksArr;
    int currNum;
    int const timeToClear;
    int currTime;
} stack_to_clear;

void *stacksArr[MAX_STACKS_TO_CLEAR];
stack_to_clear stack_list = {stacksArr, 0, 20, 0};
uthreads_list ulist = {NULL, NULL, NULL};
uthread_t *curr_uthread_exe;

// uthread_t *uthreads[MAX_THREADS];
// int uthread_count = 0;
// int uthread_curr = 0;

void *create_stack(int size) {
    void *stack;
    stack = mmap(
        NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0
    );
    return stack;
}

void uthread_scheduler() {
    int err;
    ucontext_t *cur_ctx, *next_ctx;
    stack_list.currTime++;
    if (stack_list.currTime == stack_list.timeToClear) {
        for (int i = 0; i < stack_list.currNum; i++) {
            munmap(stack_list.stacksArr[i], STACK_SIZE);
        }
        stack_list.currTime = 0;
        stack_list.currNum = 0;
    }
    // cur_ctx = &(uthreads[uthread_curr]->uctx);
    // uthread_curr = (uthread_curr + 1) % uthread_count;
    cur_ctx = &ulist.curr->uctx;
    next_ctx = &ulist.curr->next->uctx;
    ulist.curr = ulist.curr->next;
    // next_ctx = &(uthreads[uthread_curr]->uctx);
    err = swapcontext(cur_ctx, next_ctx);
    if (err == -1) {
        printf("error in uthread_schedulr: %s", strerror(errno));
        exit(1);
    }
    // printf("%s\n", ulist.curr->name);
}

void start_thread() {
    uthread_t *curr = ulist.curr;
    curr->thread_func(curr->arg);
    uthread_t *temp = ulist.first;
    while (temp->next != curr) {
        temp = temp->next;
    }
    temp->next = curr->next;
    if (curr == ulist.last) {
        ulist.last = temp;
    }
    stack_list.stacksArr[stack_list.currNum] = curr->stack;
    stack_list.currNum++;
    uthread_scheduler();
    /*
    int i;
    ucontext_t *ctx;
    for (i = 0; i < uthread_count; i++) {
        ctx = &uthreads[i]->uctx;
        char* stack_from = ctx->uc_stack.ss_sp;
        char* stack_to = ctx->uc_stack.ss_sp + ctx->uc_stack.ss_size;

        char* temp = (char*)&i;
        if (stack_from <=  temp && temp <= stack_to) {
            uthreads[i]->thread_func(uthreads[i]->arg);
        }
    }
    */
}

void uthread_create(
    uthread_t **uthread, char *name, void (*thread_func)(void *), void *arg
) {
    char *stack;
    uthread_t *new_ut;
    int err;

    assert(name);

    stack = create_stack(STACK_SIZE);
    mprotect(stack + PAGE_SIZE, STACK_SIZE - PAGE_SIZE, PROT_READ | PROT_WRITE);

    new_ut = (uthread_t *)(stack + STACK_SIZE - sizeof(uthread_t));

    err = getcontext(&new_ut->uctx);
    if (err == -1) {
        printf(
            "err in uthread_create getcontext failed: %s\n", strerror(errno)
        );
        exit(2);
    }
    new_ut->stack = stack;
    new_ut->uctx.uc_stack.ss_sp = stack;
    new_ut->uctx.uc_stack.ss_size = STACK_SIZE - sizeof(uthread_t);
    new_ut->uctx.uc_link = NULL;

    makecontext(&new_ut->uctx, start_thread, 0);

    new_ut->thread_func = thread_func;
    new_ut->arg = arg;
    strncpy(new_ut->name, name, NAME_SIZE);
    new_ut->next = ulist.first;
    ulist.last->next = new_ut;
    ulist.last = new_ut;
    // uthreads[uthread_count] = new_ut;
    // uthread_count++;
    *uthread = new_ut;
}

void uthread(void *arg) {
    for (int i = 0; i < 2; i++) {
        printf("hello from: %s\n", (char *)arg);
        sleep(1);
        uthread_scheduler();
    }
    return;
}
int ones = 0;
int main() {

    uthread_t main_thread;
    // uthreads[0] = &main_thread;
    // uthread_count++;
    ulist.curr = &main_thread;
    ulist.first = &main_thread;
    ulist.last = &main_thread;
    uthread_t *ut0;
    uthread_t *ut1;
    strncpy(main_thread.name, "user thread main", NAME_SIZE);
    main_thread.next = NULL;
    getcontext(&main_thread.uctx);
    // if (!ones) {
    uthread_create(&ut0, "user thread zero", uthread, "000 000");
    uthread_create(&ut1, "user thread one", uthread, "111 111");
    uthread_create(&ut1, "user thread two", uthread, "222 222");
    ones = 1;
    //}
    int a = 0;
    while (a < 100) {
        uthread_scheduler();
        //    printf("main\n");
        a++;
    }
    printf("main\n");
    return 0;
}
