/*
 * Copyright (c) 2014 Jozsef Galajda <jgalajda@pannongsm.hu>
 * All rights reserved.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>

#include "qlog.h"
#include "qlog_ext.h"
#include "qlog_internal.h"
#include "qlog_debug.h"
#include "qlog_display.h"
#include "qlog_display_debug.h"
#include "qlog_utils.h"


int start = 0;
int threads_started = 0;
pthread_t *threads;

void test2(){
    qlog_init(15);
    qlog_log("alma1");
    qlog_log("alma2");
    qlog_log("alma3");
    qlog_log("alma4");
    qlog_log("alma5");
    qlog_log("alma6");
    qlog_log("alma7");
    qlog_log("alma8");
    qlog_log("alma9");
    qlog_dbg_print_buffer(stderr, qlog_dbg_get_default_buffer());
}

pid_t gettid(void){
    return syscall( __NR_gettid );
}



void* thread_routine(void* thread_arg){
    int i = 0;
    int proc_num = (int)(long) thread_arg;
    char thr_name[128];
    char func_name[128];


    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(proc_num, &set);
    if (sched_setaffinity(gettid(), sizeof(cpu_set_t), &set)){
        return NULL;
    }

    memset(thr_name, 0, sizeof(thr_name));
    memset(func_name, 0, sizeof(func_name));
    snprintf(thr_name, sizeof(thr_name) - 1, "thread_%d", proc_num);
    snprintf(func_name, sizeof(func_name) - 1, "thread_routine_%d", proc_num);

    printf("thread %d has been started\n", proc_num);
    while (start == 0) {
        sleep(1);
    }
    printf("thread %d starts logging\n", proc_num);

    for (i = 0; i < 100; i++){
        qlog_log_long(thr_name, func_name, __LINE__, "alma alma alma alma alma alma alma alma alma alma alma alma papaya");
    }

    return NULL;
}

int setup(){
    int proc_num = 0;
    int i = 0;

    proc_num = (int)sysconf( _SC_NPROCESSORS_ONLN );
    if (proc_num < 0){
        return -1;
    }

    threads_started = proc_num;

    threads = (pthread_t*) malloc(sizeof(pthread_t) * proc_num);
    if (threads == NULL){
        return -1;
    }

    printf( "Starting %d threads...\n", proc_num);
    for (i = 0; i < proc_num; i++){
        if (pthread_create(&threads[i], NULL, thread_routine, (void *)(long)i )){
            i++;
            threads_started = i;
            break;
        }
    }
    printf("threads started: %d\n", threads_started);

    free(threads);
    return 0;
}

void test5(){
    qlog_init(10);
    qlog_thread_init("main thread");
    QLOG_ENTRY;
    QLOG("Test log: alma1");
    QLOG_HEX(&start, 87);
    QLOG("Test log: alma2");
    QLOG("Test log: alma3");
    QLOG_BT;
    QLOG("Test log: alma4");
    QLOG_LEAVE;
    qlog_display_print_buffer(stdout);
    /*qlog_start_server();
    qlog_wait_for_server();*/
}

void* thread_routine2(void* thread_arg){
    int i = 0;
    const char* thr_name = (const char*) thread_arg;

    printf("thread %s has been started\n", thr_name);
    qlog_thread_init(thr_name);
    while(1){
        QLOG("this is a simple log message");
        QLOG_BT;
        QLOG_HEX(&i, 85);
    }
    return NULL;
}


void test6(void){
    pthread_t thr1, thr2;
    char* thr1_name = "Thread alma";
    char* thr2_name = "Thread papaya";

    qlog_init(50);
    pthread_create(&thr1, NULL, thread_routine2, (void*)thr1_name);
    pthread_create(&thr2, NULL, thread_routine2, (void*)thr2_name);
    qlog_start_server();
    qlog_wait_for_server();

}


int main(){
    test6();
    return 0;
}
