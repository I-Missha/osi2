#include <sys/ucontext.h>
#include <time.h>
#define _GNU_SOURCE
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/futex.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <time.h>
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
    uint8_t isSleep;
    uint64_t timeToSleep;
    clock_t start;
} uthread_t;

typedef struct uthreads_list {
    uthread_t *first;
    uthread_t *last;
    uthread_t *curr;
} uthreads_list;

typedef struct stack_to_clear {
    void *stacksArr[MAX_STACKS_TO_CLEAR];
    int currNum;
    int const timeToClear;
    int currTime;
} stack_to_clear;

stack_to_clear stack_list = {{}, 0, 30, 0};
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

int isSchduler_active = 0;
void uthread_scheduler() {
    isSchduler_active = 1;
    int err;
    ucontext_t *cur_ctx, *next_ctx;
    stack_list.currTime++;
    if (stack_list.currTime == stack_list.timeToClear) {
        for (int i = 0; i < stack_list.currNum; i++) {
            if (stack_list.stacksArr[i] == NULL)
                continue;
            printf("going to munmap stack: %p\n", stack_list.stacksArr[i]);
            if (munmap(stack_list.stacksArr[i], STACK_SIZE) == -1) {
                exit(1);
            }
        }
        stack_list.currTime = 0;
        stack_list.currNum = 0;
    }
    // cur_ctx = &(uthreads[uthread_curr]->uctx);
    // uthread_curr = (uthread_curr + 1) % uthread_count;
    // ToDo: find uthread that we can wake up
    uthread_t *utid_curr = ulist.curr;
    ucontext_t save_ctx = ulist.curr->uctx;
    getcontext(&save_ctx);
    uthread_t *utid_iter = utid_curr;
    if (utid_iter->next == NULL) {
        utid_iter = ulist.first;
    }
    utid_iter = utid_curr->next;
    while (utid_iter->isSleep &&
           ((clock() - utid_iter->start) / CLOCKS_PER_SEC <
            utid_iter->timeToSleep)) {
        if (utid_iter->next == NULL) {
            utid_iter = ulist.first;
            continue;
        }
        utid_iter = utid_iter->next;
    }
    /*printf(*/
    /*    "diff: %ld \ntime to sleep: %ld\n",*/
    /*    (clock() - utid_iter->start) / CLOCKS_PER_SEC,*/
    /*    utid_iter->timeToSleep*/
    /*);*/
    utid_iter->isSleep = 0;
    cur_ctx = &utid_curr->uctx;
    next_ctx = &utid_iter->uctx;
    ulist.curr = utid_iter;

    // next_ctx = &(uthreads[uthread_curr]->uctx);
    isSchduler_active = 0;
    // printf("%s", utid_iter->name);
    err = swapcontext(cur_ctx, next_ctx);
    if (utid_curr->isSleep) {
        isSchduler_active = 1;
        setcontext(&save_ctx);
    }
    if (err == -1) {
        printf("error in uthread_schedulr: %s", strerror(errno));
        exit(1);
    }
    // printf("hello\n");
    //  printf("%s\n", ulist.curr->name);
}

void uthread_sleep(uint64_t sleep_time) {
    ulist.curr->isSleep = 1;
    ulist.curr->timeToSleep = sleep_time;
    ulist.curr->start = clock();
    uthread_scheduler();
}

uthread_t *saved;
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
}

ucontext_t sched;
void uthread_sched_yield() {
    while (1) {
        uthread_scheduler();
    }
}

void uthread_sched_init() {
    getcontext(&sched);
    void *stack = create_stack(STACK_SIZE);
    mprotect(stack, STACK_SIZE, PROT_READ | PROT_WRITE);
    sched.uc_stack.ss_sp = stack;
    sched.uc_stack.ss_size = STACK_SIZE;
    sched.uc_link = NULL;
    makecontext(&sched, uthread_sched_yield, 0);
}

void custom_sleep() {
    for (uint64_t i = 0; i < 1000000000; i++) {
    }
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
    new_ut->isSleep = 0;
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
    for (int i = 0; i < 3; i++) {
        printf("hello from: %s\n", (char *)arg);
        uthread_sleep(1);
        //  printf("sleep\n");
        // custom_sleep();
    }
    return;
}

void alarm_sig_handler() {
    struct sigaction alarm_sigaction;
    alarm_sigaction.sa_handler = alarm_sig_handler;
    sigaction(SIGALRM, &alarm_sigaction, NULL);
    alarm(1);
    if (isSchduler_active) {
        return;
    }
    setcontext(&sched);
    // uthread_scheduler();
}

int ones = 0;
int main() {
    struct sigaction alarm_sigaction;
    alarm_sigaction.sa_handler = alarm_sig_handler;
    sigaction(SIGALRM, &alarm_sigaction, NULL);
    uthread_sched_init();
    alarm(1);
    uthread_t main_thread;
    ulist.curr = &main_thread;
    ulist.first = &main_thread;
    ulist.last = &main_thread;
    uthread_t *ut0;
    uthread_t *ut1;
    strncpy(main_thread.name, "user thread main", NAME_SIZE);
    main_thread.next = NULL;
    main_thread.isSleep = 0;
    getcontext(&main_thread.uctx);
    // if (!ones) {
    uthread_create(&ut0, "user thread zero", uthread, "000 000");
    uthread_create(&ut1, "user thread one", uthread, "111 111");
    uthread_create(&ut1, "user thread two", uthread, "222 222");
    ones = 1;
    //}
    uthread_sleep(10);
    /*for (int i = 0; i < 10; i++) {*/
    /*    custom_sleep();*/
    /*    uthread_scheduler();*/
    /*}*/
    return 0;
}
