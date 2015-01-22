/*
 * Copyright (c) 2014 Jozsef Galajda <jgalajda@pannongsm.hu>
 * All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <execinfo.h>
#include <pthread.h>
#include "qlog.h"
#include "qlog_internal.h"
#include "qlog_ext.h"

struct {
    qlog_ext_event_info_t events[256];
    unsigned int index;
    qlog_ext_event_type_t next_event_type;
    unsigned char initialized;
    pthread_spinlock_t lock;
} qlog_ext_events;

int qlog_ext_init(void){
    int spin_res = 0;
    int ret = QLOG_RET_ERR;
    memset(qlog_ext_events.events, 0, sizeof(qlog_ext_events.events));
    qlog_ext_events.index = 0;
    qlog_ext_events.initialized = 0;
    qlog_ext_events.next_event_type = QLOG_EXT_EVENT_TYPE_DYNAMIC_START;
    spin_res = pthread_spin_init(&qlog_ext_events.lock, PTHREAD_PROCESS_PRIVATE);
    if (spin_res == 0){
        ret = qlog_ext_register_built_in_events();
        if (ret == QLOG_RET_OK) {
            qlog_ext_events.initialized = 1;
        }
    }
    return ret;
}

int qlog_ext_register_built_in_events(void){
    int spin_res = 0;
    int ret = QLOG_RET_ERR;

    spin_res = pthread_spin_lock(&qlog_ext_events.lock);
    if (spin_res == 0){
        qlog_ext_events.events[qlog_ext_events.index].event_type = QLOG_EXT_EVENT_TYPE_BT;
        qlog_ext_events.events[qlog_ext_events.index].print_callback = qlog_ext_display_bt;
        qlog_ext_events.index++;

        qlog_ext_events.events[qlog_ext_events.index].event_type = QLOG_EXT_EVENT_TYPE_HEXDUMP;
        qlog_ext_events.events[qlog_ext_events.index].print_callback = qlog_ext_display_hex_dump;
        qlog_ext_events.index++;

        spin_res = pthread_spin_unlock(&qlog_ext_events.lock);
        if (spin_res == 0){
            ret = QLOG_RET_OK;
        }
    }
    return ret;
}

qlog_ext_event_type_t qlog_ext_register_event(qlog_ext_print_cb_t print_callback){
    int i = 0;
    int ret = QLOG_EXT_EVENT_TYPE_NONE;
    int spin_res = 0;
    int dup_found = 0;

    if (qlog_ext_events.initialized){
        spin_res = pthread_spin_lock(&qlog_ext_events.lock);
        if (spin_res == 0){
            for (i = 0; i < qlog_ext_events.index; i++){
                if (qlog_ext_events.events[i].print_callback == print_callback){
                    dup_found = 1;
                    break;
                }
            }
            if (dup_found == 1){
                ret = qlog_ext_events.events[i].event_type;
            } else {
                qlog_ext_events.events[qlog_ext_events.index].event_type = qlog_ext_events.next_event_type;
                qlog_ext_events.events[qlog_ext_events.index].print_callback = print_callback;
                qlog_ext_events.index++;
                ret = qlog_ext_events.next_event_type++;
            }

            spin_res = pthread_spin_unlock(&qlog_ext_events.lock);
            if (spin_res != 0){
                qlog_ext_events.initialized = 0;
            }
        }
    }
    return ret;
}


int qlog_ext_log(qlog_ext_event_type_t event_type, void* ext_data, size_t data_size, const char* message){
    return qlog_ext_log_long_id(qlog_internal_get_default_buf_id(),
            event_type, ext_data, data_size, NULL, NULL, 0, message);
}

int qlog_ext_log_id(qlog_buffer_id_t buffer_id,
        qlog_ext_event_type_t event_type,
        void* ext_data,
        size_t data_size,
        const char* message)
{
    return qlog_ext_log_long_id(buffer_id, event_type, ext_data, data_size, NULL, NULL, 0, message);
}

int qlog_ext_log_long(qlog_ext_event_type_t event_type,
        void* ext_data,
        size_t data_size,
        const char* thread,
        const char* function_name,
        int line_number,
        const char* message)
{
    return qlog_ext_log_long_id(qlog_internal_get_default_buf_id(),
            event_type, ext_data, data_size, thread, function_name, line_number, message);
}

int qlog_ext_log_long_id(qlog_buffer_id_t buffer_id,
        qlog_ext_event_type_t event_type,
        void* ext_data,
        size_t data_size,
        const char* thread_name,
        const char* function_name,
        int line_number,
        const char* message)
{
    const char *thread_name_p = NULL;
    int res = QLOG_RET_ERR;
    qlog_buffer_t* buffer = qlog_internal_get_buffer_by_id(buffer_id);

    if (qlog_internal_is_lib_inited()
            && qlog_internal_is_logging_enabled()
            && qlog_ext_events.initialized == 1
            && qlog_ext_event_type_is_valid(event_type)
            && buffer != NULL)
    {
        if (thread_name == NULL) {
            if (qlog_internal_get_thread_name()[0] != '0'){
                thread_name_p = qlog_internal_get_thread_name();
            }
        } else {
            thread_name_p = thread_name;
        }

        res = qlog_log_internal(buffer, thread_name_p, function_name, line_number, message, ext_data, data_size, event_type);
    }
    return res;
}

qlog_ext_print_cb_t qlog_ext_get_print_cb(qlog_ext_event_type_t ext_event_type){
    qlog_ext_print_cb_t ret = NULL;
    int spin_res = 0;
    int i = 0;

    spin_res = pthread_spin_lock(&qlog_ext_events.lock);
    if (spin_res == 0){

        for (i = 0; i < qlog_ext_events.index; i++){
            if (qlog_ext_events.events[i].event_type == ext_event_type){
                ret = qlog_ext_events.events[i].print_callback;
                break;
            }
        }

        spin_res = pthread_spin_unlock(&qlog_ext_events.lock);
        if (spin_res != 0){
            ret = NULL;
            qlog_ext_events.initialized = 0;
        }
    } else {
        qlog_ext_events.initialized = 0;
    }
    return ret;
}

int qlog_ext_event_type_is_valid(qlog_ext_event_type_t event_type){
    return ((event_type > QLOG_EXT_EVENT_TYPE_NONE && event_type <= QLOG_EXT_EVENT_TYPE_LAST) ||
            (event_type >= QLOG_EXT_EVENT_TYPE_DYNAMIC_START && event_type < qlog_ext_events.next_event_type));
}
