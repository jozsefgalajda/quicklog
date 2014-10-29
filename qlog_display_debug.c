/*
 * Copyright (c) 2014 Jozsef Galajda <jgalajda@pannongsm.hu>
 * All rights reserved.
 */

#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include "qlog.h"
#include "qlog_internal.h"
#include "qlog_display.h"
#include "qlog_display_debug.h"


extern qlog_buffer_t* qlog_buffers[];
extern int qlog_lib_inited;


/**
 * \brief Print the content of a buffer to a stream
 *
 * \param stream The stream into which the buffer is printed
 * \param buffer_id The id of the buffer to be printed
 *
 * Prints the log buffer contents (the log messages) into the stream.
 * The empty slots are also printed.
 */
void qlog_display_debug_print_buffer_id(FILE* stream, qlog_buffer_id_t buffer_id){
    int res = 0;
    int start = 1;
    qlog_event_t* event = NULL;
    qlog_buffer_t* buffer = NULL;

    buffer = qlog_internal_get_buffer_by_id(buffer_id);
    if (buffer){
        res = qlog_lock_buffer_internal(buffer);
        if (res){
            return;
        }
        event = buffer->head;
        while(event){
            if (event == buffer->head){
                if (start == 1){
                    start = 0;
                    qlog_display_event(stream, event);
                    event = event->next;
                } else {
                    event = NULL;
                }
            } else {
                qlog_display_event(stream, event);
                event = event->next;
            }
        }

        res = qlog_unlock_buffer_internal(buffer);
    } else {
        fprintf(stream, "The buffer is not initialized\n");
    }
}


/**
 * \brief Print the status and content of a all the buffers into a stream
 *
 * \param stream The stream into which the buffer is printed
 * \param print_status (Flag) If not 0 the status of the buffer is printed
 * \param print_events (flag) If not 0 the buffer contents are printed
 *
 * Prints the log buffer contents (the log messages) and the status of the
 * buffer into the stream provided as a paramter.
 * The flags decide what to print: only the status of the buffer,
 * only the log messag, both or none.
 */
void qlog_display_debug_print_all_buffers(FILE* stream, int print_status, int print_events){
    int i = 0;
    int start = 1;
    qlog_event_t* event = NULL;
    qlog_buffer_t* buffer = NULL;
    if (qlog_lib_inited){
        for (i = 0; i < qlog_internal_get_max_buf_num(); i++){
            fprintf(stream, "Buffer index: %d\n", i);
            buffer = qlog_internal_get_buffer_by_id(i);
            if (buffer == NULL){
                fprintf(stream, "This buffer is not initialized.\n");
            } else {
                qlog_lock_buffer_internal(buffer);
                if (print_status) {
                    fprintf(stream, "Buffer status:\n");
                    fprintf(stream, "  Buffer head         : %p\n", (void*) buffer->head);
                    fprintf(stream, "  Buffer next         : %p\n", (void*) buffer->next_write);
                    fprintf(stream, "  Buffer size         : %u\n", (unsigned int) buffer->buffer_size);
                    fprintf(stream, "  Buffer wrapped      : %d\n", buffer->wrapped);
                    fprintf(stream, "  Buffer event locked : %d\n\n", buffer->event_locked);
                }
                if (print_events) {
                    fprintf(stream, "Log messages:\n");
                    event = buffer->head;
                    start = 1;
                    while(event){
                        if (event == buffer->head){
                            if (start == 1){
                                start = 0;
                                qlog_display_event(stream, event);
                                event = event->next;
                            } else {
                                event = NULL;
                            }
                        } else {
                            qlog_display_event(stream, event);
                            event = event->next;
                        }
                    }
                }
                qlog_unlock_buffer_internal(buffer);
            }
        }
    } else {
        fprintf(stream, "The qlog library has not been initialized\n");
    }
}

