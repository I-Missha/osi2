#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#define SIZE_OF_STR 16


void *mythread(void *arg) {
    char* str = (char*)malloc(SIZE_OF_STR * sizeof(char));
    snprintf(str, 13, "%s", "hello world\n");
    pthread_cleanup_push(free, str);
    //pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    int counter = 0;
    while (1) {
        //printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
        printf("%s", str);
        counter++;
    }
    pthread_cleanup_pop(1);
    return 0;
}

int main() {
    pthread_t tid;
    int err;
    pthread_attr_t attr;
    pthread_attr_setdetachstate(&attr,  PTHREAD_CREATE_DETACHED);
    printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());
    err = pthread_create(&tid, NULL, mythread, NULL);
    if (err) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }
    pthread_cancel(tid);
    pthread_join(tid, NULL);
    return 0;
}

