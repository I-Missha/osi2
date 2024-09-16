#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <unistd.h>

char hello[12] = {"hello world"};
void *mythread(void *arg) {
	printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
    sleep(3);
    printf("mythread exit\n");
    return hello;
}
int main() {
	pthread_t tid;
	int err;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());
	err = pthread_create(&tid, NULL, mythread, NULL);
	if (err) {
	    printf("main: pthread_create() failed: %s\n", strerror(err));
	    return -1;
	}
    char* thread_return;
    pthread_join(tid, (void**)&thread_return);
    printf("exite with status: %s\n", thread_return);
    printf("main thread exit\n");
	return 0;
}

