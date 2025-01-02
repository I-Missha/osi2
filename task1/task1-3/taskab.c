#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct Group {
    int a;
    char* str;
}Group;


void *mythread(void *arg) {
    printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
    Group* a = (Group*)arg;
    sleep(1);
    int ret = printf("arg: %d, %s", a->a, a->str);
    printf("return from print: %d\n", ret);
    return 0;
}

void *mytestfunc(void *arg) {
    pthread_t tid;
    int err;
    pthread_attr_t attr;
    pthread_attr_setdetachstate(&attr,  PTHREAD_CREATE_DETACHED);
    Group group = {0, "hello world\n"};
    err = pthread_create(&tid, &attr, mythread, &group);
    return 0;
}

int main() {
    pthread_t tid;
    int err;
    pthread_attr_t attr;
    pthread_attr_setdetachstate(&attr,  PTHREAD_CREATE_DETACHED);
    Group group = {0, "hello world\n"};
    printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());
    err = pthread_create(&tid, &attr, mytestfunc, &group);
    if (err) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }
    pthread_exit(NULL);
    return 0;
}

