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
int test8_run  = 0;

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
    printf("================================================================================\n");
    printf(" Test #5\n");
    printf("================================================================================\n");
    printf("(*) Initialize library with size of 10\n\n");
    qlog_init(10);
    qlog_thread_init("main thread");

    qlog_dbg_print_buffers(stdout);

    printf("(*) Adding log messages (8)\n\n");
    QLOG_ENTRY;
    QLOG("Test log: alma1");
    QLOG_HEX(&start, 87);
    QLOG("Test log: alma2");
    QLOG("Test log: alma3");
    QLOG_BT;
    QLOG("Test log: alma4");
    QLOG_LEAVE;
    printf("================================================================================\n");
    printf("================================================================================\n");
    qlog_dbg_print_buffers(stdout);
    qlog_reset();
    QLOG("Test log2: alma1");
    QLOG("Test log2: alma2");
    QLOG("Test log2: alma3");
    QLOG("Test log2: alma4");
    QLOG("Test log2: alma5");
    QLOG("Test log2: alma6");
    QLOG_BT;
    QLOG_HEX(&start, 100);
    QLOG("Test log2: alma7");
    QLOG("Test log2: alma8");
    QLOG("Test log2: alma9");
    QLOG("Test log2: alma10");
    printf("================================================================================\n");
    printf("================================================================================\n");
    qlog_dbg_print_buffers(stdout);
    qlog_reset();
    QLOG("Test log2: alma1");
    QLOG("Test log2: alma2");
    QLOG("Test log2: alma3");
    QLOG("Test log2: alma4");
    QLOG("Test log2: alma5");
    QLOG("Test log2: alma6");
    QLOG_BT;
    QLOG_HEX(&start, 100);
    QLOG("Test log2: alma7");
    QLOG("Test log2: alma8");
    QLOG("Test log2: alma9");
    QLOG("Test log2: alma10");
    printf("================================================================================\n");
    printf("================================================================================\n");
    qlog_dbg_print_buffers(stdout);
    qlog_start_server();
    qlog_wait_for_server();
    qlog_cleanup();
}


void test7(int type){
    int i = 0, j = 0;
    char* buffer;
    qlog_init(50);
    qlog_thread_init("test_6_main_thread");
    for (i = 0; i < 10; i++){
        for (j = 0; j< 1500; j++){
            switch (type){
                case 1:
                    QLOG("log entry");
                    break;
                case 2:
                    QLOG_HEX(&buffer, 15);
                    break;
                case 3:
                    QLOG_BT;
                    break;
                case 4:
                    QLOG("log entry sdlkfjsdklfjsdlkfjsdklfj");
                    QLOG_HEX(&buffer, 98);
                    QLOG_BT;
                default:
                    break;
            }
        }
        /*qlog_display_print_buffer(stdout);*/
        qlog_reset();
    }
    qlog_cleanup();
}

void* test8_thr(void* data){
    char* thread_name = (char*) data;
    struct timespec sleep_time;
    struct timespec remaining_time;
    unsigned long counter = 0;
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 1000000;

    qlog_thread_init(thread_name);
    printf("Thread started: %s\n", thread_name);
    while(1){
        if (test8_run == 0) {
            printf("Loop count in thread %s: %lu\n", thread_name, counter);
            break;
        }
        counter++;
        QLOG("thread message");
        QLOG_HEX(thread_name, 10);
        QLOG_BT;
        nanosleep(&sleep_time, &remaining_time);
    }
    return NULL;
}

void test6(){
    qlog_init(10);
    qlog_thread_init("main thread");
    QLOG_BT;
    qlog_display_print_buffer(stdout);
    qlog_cleanup();
}

void test8(int duration, int reset){
    pthread_t thr1, thr2;
    char* thr1_name = "Thread1";
    char* thr2_name = "Thread2";
    int i = 0;

    qlog_init(13);
    test8_run = 1;
    pthread_create(&thr1, NULL, test8_thr, (void*)thr1_name);
    pthread_create(&thr2, NULL, test8_thr, (void*)thr2_name);
    for (i = 0; i< duration; i++){
        printf("%d ", i);
        if (reset){
            qlog_reset();
            printf(" r ");
        }
        fflush(stdout);
        sleep(1);
    }
    test8_run = 0;
    pthread_join(thr1, NULL);
    pthread_join(thr2, NULL);
    qlog_display_print_buffer(stdout);
    qlog_cleanup();
}


int main(){
    test6();
    return 0;
}
