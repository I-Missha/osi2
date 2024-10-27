#define _GNU_SOURCE
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <ucontext.h>
#include <stdint.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <err.h>

#define PAGE_SIZE 4096
#define STACK_SIZE PAGE_SIZE * 4


typedef unsigned long int mythread_t;
typedef void* (*mythread_routine)(void*);

typedef struct _mythread {
    int mythread_id;
    mythread_routine routine;
    void* arg;
    void* retval;
    volatile int finished;
    volatile int joined;
    volatile uint32_t joinedFutex;
    volatile uint32_t finishedFutex;
    volatile int isDetached;
    ucontext_t ctx;
    volatile int isCancelled;
}mythread_struct_t;

static uint32_t *futex1, *futex2, *iaddr;

static int futex(uint32_t *uaddr, int futex_op, uint32_t val,
                 const struct timespec *timeout, uint32_t *uaddr2, uint32_t val3) {
    return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

static void fwait(uint32_t *futexp) {
    long s;
    s = futex(futexp, FUTEX_WAIT, 0, NULL, NULL, 0);
    if (s == -1 && errno != EAGAIN)
        err(EXIT_FAILURE, "futex FUTEX_WAIT");
}

static void fpost(uint32_t *futexp) {
    long s;

    s = futex(futexp, FUTEX_WAKE, 1, NULL, NULL, 0);
    if (s  == -1)
        err(EXIT_FAILURE, "futex FUTEX_WAKE");
}

void mythread_detach(mythread_struct_t* mythread) {
    mythread->isDetached = 1;
}

void mythread_testcancel(mythread_struct_t* thread) {
    if (thread->isCancelled) {
        setcontext(&thread->ctx);
    }
}

int start_routine(void* arg) {

    mythread_struct_t* thread = (mythread_struct_t*)arg;

    printf("mythread started doing a routine.\n thread_id: %d\n", thread->mythread_id);
    int err = getcontext(&thread->ctx);
    if (!thread->isCancelled) {
        thread->retval = thread->routine(thread->arg);
    }
    thread->finished = 1;
    fpost(&thread->finishedFutex);
    //ToDo: make futex
    printf("mythread started waiting stage for join.\n thread_id: %d\n", thread->mythread_id);

    //if (!thread->joined && !thread->isDetached) {
    //    fwait(&thread->joinedFutex);
    //}
    //while(!thread->joined && !thread->isDetached) {
    //    sleep(1);
    //}
    printf("routine finished.\n thread_id: %d\n", thread->mythread_id);
    return 0;
}


void* create_stack(int size) {
    void* stack;
    stack = mmap(NULL, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0);
    return stack;
}


int mythread_create(mythread_struct_t** mythread, mythread_routine routine, void* arg) {
    static int thread_num = 0;
    mythread_struct_t *thread; 
    thread_num++;
    int child_pid;
    void *child_stack;

    child_stack = create_stack(STACK_SIZE);

    mprotect(child_stack + PAGE_SIZE, STACK_SIZE - PAGE_SIZE, PROT_READ | PROT_WRITE);

    thread = (mythread_struct_t*)(child_stack + STACK_SIZE - sizeof(mythread_struct_t));
    thread->mythread_id = thread_num;
    thread->arg = arg;
    thread->routine = routine;
    thread->notFinished = 1;
    thread->joined = 0;
    thread->isDetached = 1;
    int err = clone(start_routine,  (void*)thread, CLONE_VM|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|CLONE_CHILD_CLEARTID, thread);
    if (err == -1) {
        printf("clone failed: %s\n", strerror(errno));
        exit(-1);
    }
    *mythread = thread;
    return 0;
}

void mythread_cancel(mythread_struct_t* thread) {
    thread->isCancelled = 1; 
}

void* mythread_join(mythread_struct_t* mythread) {
    printf("waiting for thread %d to be finished\n", mythread->mythread_id);
    fwait(&mythread->finishedFutex);
    //while(!mythread->finished) {
    //    usleep(1);
    //}

    printf("mythread %d finished.\n", mythread->mythread_id);
    printf("mythread return value is '%s'", (char*)mythread->retval);
    mythread->joined = 1;
    fpost(&mythread->joinedFutex);
    return mythread->retval;
}

void* mythread(void* arg) {
    for (int i = 0; i < 2; i++) {
        printf("hello from mythread\n");
        sleep(1);
    }
    return "goodbye from my thread"; 
}


int main() {
    mythread_struct_t* tid;
    mythread_create(&tid, mythread, "hello"); 
    mythread_join(tid);
    return 0;
}


