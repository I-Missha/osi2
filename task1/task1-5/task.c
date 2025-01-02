#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

static void myexit() {
    printf("\nhello from my exit\n"); 
    exit(0) ;
}


void* block_routine(void* arg) {
    sigset_t set;
    sigfillset(&set);
    int s = pthread_sigmask(SIG_BLOCK, &set, NULL);
    
    sleep(4);
    return 0;
}

void* int_routine(void* arg) {
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGINT);
    int s = pthread_sigmask(SIG_SETMASK, &set, NULL);
    while(1){}
    return 0;
}


void* quit_routine(void* arg) {
    sigset_t set;
    //sigfillset(&set);
    sigemptyset(&set);
    sigaddset(&set, SIGQUIT);
    int s = pthread_sigmask(SIG_SETMASK, &set, NULL);
    int sig;
    while(1){
        sigwait(&set, &sig);
        printf("\nhello from quit_routine\n");
    }
    return 0;
}

int main() {

    pthread_t tid_sigblock , tid_sigint, tid_sigquit;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    //pthread_create(&tid_sigblock, &attr, block_routine, NULL);
    pthread_create(&tid_sigquit, &attr, quit_routine, NULL);
    struct sigaction ex;
    ex.sa_handler = myexit;
    sigaction(SIGINT, &ex, NULL);
    pthread_create(&tid_sigint, &attr, int_routine, NULL);
    
    sigset_t set;
    sigfillset(&set);
    int s = pthread_sigmask(SIG_SETMASK, &set, NULL);


    while(1){}
    //sleep(4);
    return 0;
}

