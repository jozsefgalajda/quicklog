/*
 * Copyright (c) 2014 Jozsef Galajda <jgalajda@pannongsm.hu>
 * All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <execinfo.h>
#include "qlog.h"
#include "qlog_internal.h"


void qlog_ext_display_hex_dump(FILE* stream, void* datap, size_t size){
    unsigned int i = 0, j = 0;
    char temp[8] = {0};
    char buffer[128] = {0};
    char *ascii;
    char* data = (char*)datap;

    memset(buffer, 0, sizeof(buffer));
    fprintf(stream, "\tHexdump of %lu bytes:\n", size);
    fprintf(stream, "\t        +0          +4          +8          +c            0   4   8   c   \n");
    fprintf(stream, "\t        ------------------------------------------------  ----------------\n");

    ascii = buffer + 58;
    memset(buffer, ' ', 58 + 16);
    buffer[58 + 16] = '\n';
    buffer[58 + 17] = '\0';
    buffer[0] = '+';
    buffer[1] = '0';
    buffer[2] = '0';
    buffer[3] = '0';
    buffer[4] = '0';
    for (i = 0, j = 0; i < size; i++, j++) {
        if (j == 16) {
            fprintf(stream, "\t%s", buffer);
            memset(buffer, ' ', 58 + 16);
            sprintf(temp, "+%04x", i);
            memcpy(buffer, temp, 5);
            j = 0;
        }

        sprintf(temp, "%02x", 0xff & data[i]);
        memcpy(buffer + 8 + (j * 3), temp, 2);
        if ((data[i] > 31) && (data[i] < 127)){
            ascii[j] = data[i];
        } else{
            ascii[j] = '.';
        }
    }

    if (j != 0) {
        fprintf(stream, "\t%s", buffer);
    }
}

void qlog_ext_display_bt(FILE* stream, void* data, size_t size){
    size_t i;
    char **strings;
    size_t num_of_stacks = size / sizeof(void*);
    void** bt = (void**)data;

    strings = backtrace_symbols(bt, num_of_stacks);

    for (i = 0; i < num_of_stacks; i++){
        fprintf(stream, "\tframe#%-3lu: %s\n", i, strings[i]);
    }

    free (strings);
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
    qlog_ext_print_cb_t fn = NULL;
    switch (ext_event_type){
        case QLOG_EXT_EVENT_HEXDUMP:
            fn = qlog_ext_display_hex_dump;
            break;
        case QLOG_EXT_EVENT_BT:
            fn = qlog_ext_display_bt;
            break;
        case QLOG_EXT_EVENT_CUSTOM:
            break;
        default:
            break;
    }
    return fn;
}

int qlog_ext_event_type_is_valid(qlog_ext_event_type_t event_type){
    return (event_type != QLOG_EXT_EVENT_NONE);
}
