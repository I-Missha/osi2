#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *arg) {
    printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
    for (int i = 0; i < 100000; i++){}
    return 0;
}
int main() {
    pthread_t tid;
    int err;
    pthread_attr_t attr;
    pthread_attr_setdetachstate(&attr,  PTHREAD_CREATE_DETACHED);
    printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());
    while (1) {
        err = pthread_create(&tid, NULL, mythread, NULL);
        pthread_detach(tid);
        if (err) {
            printf("main: pthread_create() failed: %s\n", strerror(err));
            return -1;
        }
    }
    return 0;
}

