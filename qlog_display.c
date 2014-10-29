/*
 * Copyright (c) 2014 Jozsef Galajda <jgalajda@pannongsm.hu>
 * All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#include "qlog.h"
#include "qlog_internal.h"
#include "qlog_display.h"

extern int qlog_lib_inited;

void qlog_display_format_timestamp(char* buffer, size_t size, const struct timeval* t){
    struct tm bdt;
    size_t len = 0;

    memset(&bdt, 0, sizeof(bdt));
    memset(buffer, 0, size);

    localtime_r(&t->tv_sec, &bdt);
    strftime(buffer, size - 1, "%m/%d/%y %H:%M:%S", &bdt);
    len = strlen(buffer);
    snprintf(buffer + len, size - len - 1, ".%-6ld", t->tv_usec);
}

void qlog_display_format_event_str(const qlog_event_t* event, char* buffer, size_t buffer_size){
    char timestamp_str[30];

    if (event == NULL || buffer == NULL || buffer_size == 0){
        return;
    }

    memset(buffer, 0, buffer_size);
    qlog_display_format_timestamp(timestamp_str, sizeof(timestamp_str), &event->timestamp);

    snprintf(buffer, buffer_size - 1,
            "%s [%s:%s:%u]: %s",
            timestamp_str,
            event->thread_name[0] != '\0' ? event->thread_name : "-",
            event->function_name[0] != '\0' ? event->function_name : "-",
            event->line_number,
            event->message[0] != '\0' ? event->message : "-");
}


void qlog_display_event(FILE* stream, const qlog_event_t* event){
    char buffer[256];

    memset(buffer, 0, sizeof(buffer));
    qlog_display_format_event_str(event, buffer, sizeof(buffer));
    fprintf(stream, "%s\n", buffer);
    if (event->ext_event_type != QLOG_EXT_EVENT_NONE && event->ext_data && 
        event->ext_data_size > 0 && event->ext_print_cb){
        fprintf(stream, "\n");
        event->ext_print_cb(stream, event->ext_data, event->ext_data_size);
        fprintf(stream, "\n");
    }
}

/* print the defaul buffer */
void qlog_display_print_buffer(FILE* stream){
    qlog_display_print_buffer_id(stream, 0);
}

/*print a buffer with a specified id */
void qlog_display_print_buffer_id(FILE* stream, qlog_buffer_id_t buffer_id){
    qlog_event_t* event = NULL, *start = NULL;
    int res = 0;
    int first = 1;

    qlog_buffer_t* buffer = qlog_internal_get_buffer_by_id(buffer_id);
    if (stream && buffer){
        res = qlog_lock_buffer_internal(buffer);
        if (res){
            return;
        }

        if (buffer->wrapped != 0){
            event = start = buffer->next_write;
        } else {
            event = start = buffer->head;
        }

        while (event){
            if (event == start){
                if (first == 1){
                    first = 0;
                    if (event->used == 1){
                        qlog_display_event(stream, event);
                        event = event->next;
                    } else {
                        event = NULL;
                    }
                } else {
                    event = NULL;
                }
            } else {
                if (event->used){

                    qlog_display_event(stream, event);
                    event=event->next;
                } else {
                    event = NULL;
                }
            }
        }
        res = qlog_unlock_buffer_internal(buffer);
    }
}


/**
 * \brief Print the buffer list and status
 *
 * \param stream The stream to print the buffer list into
 *
 * Prints a short buffer list and status into the stream provided as a parameter.
 */
void qlog_display_print_buffer_list(FILE* stream){
    int i = 0;
    if (qlog_lib_inited && stream){
        for (i = 0; i < qlog_internal_get_max_buf_num(); i++){
            fprintf(stream, "Qlog log buffer #%d: %s\n", i, qlog_internal_get_buffer_by_id(i) ? "initialized" : "not initialized");
        }
    }
}

