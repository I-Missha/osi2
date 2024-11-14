#define _GNU_SOURCE
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
#include <threads.h>
#include <ucontext.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define STACK_SIZE PAGE_SIZE * 4
#define MYTHREAD_CANCELLED (void *)1234453
#define STACKS_NUM 100
typedef unsigned long int mythread_t;
typedef void *(*mythread_routine)(void *);

typedef struct _mythread {
    int mythread_id;
    mythread_routine routine;
    void *arg;
    void *retval;
    void *stack;
    volatile int notFinished;
    volatile int isDetached;
    volatile int isCancelled;
    ucontext_t ctx;
} mythread_struct_t;

mythread_struct_t *tid;
void mythread_detach(mythread_struct_t *mythread) {
    mythread->isDetached = 1;
}

void mythread_testcancel(mythread_struct_t *thread) {
    if (thread->isCancelled) {
        setcontext(&thread->ctx);
    }
}
thread_local ucontext_t new_context;
thread_local mythread_struct_t local_thread;
void free_stack_of_thread() {
    printf("%d\n", munmap(local_thread.stack, STACK_SIZE));
}

int start_routine(void *arg) {

    mythread_struct_t *thread = (mythread_struct_t *)arg;

    printf(
        "mythread started doing a routine. thread_id: %d\n", thread->mythread_id
    );

    int err = getcontext(&thread->ctx);

    if (!thread->isCancelled) {
        thread->retval = thread->routine(thread->arg);
    } else {
        thread->retval = MYTHREAD_CANCELLED;
    }

    if (thread->isDetached) {
        void *new_stack = (void *)malloc(PAGE_SIZE);
        getcontext(&new_context);
        new_context.uc_stack.ss_sp = new_stack;
        new_context.uc_stack.ss_size = PAGE_SIZE;
        new_context.uc_stack.ss_flags = 0;
        new_context.uc_link = NULL;
        local_thread.stack = thread->stack;
        makecontext(&new_context, free_stack_of_thread, 0);
        setcontext(&new_context);
    }
    printf("routine finished: thread_id: %d\n", thread->mythread_id);
    return 0;
}

void *create_stack(int size) {
    void *stack;
    stack = mmap(
        NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0
    );
    return stack;
}

int mythread_create(
    mythread_struct_t **mythread, mythread_routine routine, void *arg
) {
    static int thread_num = 0;
    mythread_struct_t *thread;
    thread_num++;
    int child_pid;
    void *child_stack;

    child_stack = create_stack(STACK_SIZE);

    mprotect(
        child_stack + PAGE_SIZE, STACK_SIZE - PAGE_SIZE, PROT_READ | PROT_WRITE
    );

    thread = (child_stack + STACK_SIZE - sizeof(mythread_struct_t));
    thread->stack = child_stack;
    thread->mythread_id = thread_num;
    thread->arg = arg;
    thread->routine = routine;
    thread->notFinished = 1;
    thread->isDetached = 0;
    printf("%p\n", child_stack);
    int err = clone(
        start_routine,
        child_stack + STACK_SIZE - sizeof(mythread_struct_t),
        CLONE_VM | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD |
            CLONE_CHILD_CLEARTID | CLONE_CHILD_CLEARTID,
        thread,
        &thread->notFinished,
        NULL,
        &thread->notFinished
    );
    if (err == -1) {
        printf("clone failed: %s\n", strerror(errno));
        exit(-1);
    }
    *mythread = thread;
    return 0;
}

void mythread_cancel(mythread_struct_t *thread) {
    thread->isCancelled = 1;
}

int mythread_join(mythread_struct_t *mythread, void **retval) {
    printf("waiting for thread_id: %d to be finished\n", mythread->mythread_id);
    if (mythread->isDetached) {
        printf("%d is detached\n", mythread->mythread_id);
        return -1;
    }
    while (mythread->notFinished) {
        usleep(1);
    }

    if (retval != NULL) {
        *retval = mythread->retval;
    }
    mythread_struct_t saved = *mythread;
    printf("%p\n", saved.stack);
    int err = munmap((void *)saved.stack, STACK_SIZE);
    printf("%d\n", err);
    printf("%s\n", strerror(errno));
    return 0;
}

void *mythread(void *arg) {
    for (int i = 0; i < 5; i++) {
        printf("hello from mythread\n");
        sleep(1);
        if (i == 2) {
            mythread_testcancel(tid);
        }
    }
    return "goodbye from my thread";
}

int main() {
    mythread_create(&tid, mythread, "hello");
    mythread_detach(tid);
    char retval[20];
    sleep(1);
    // mythread_cancel(tid);
    mythread_join(tid, (void *)retval);
    sleep(7);
    return 0;
}
