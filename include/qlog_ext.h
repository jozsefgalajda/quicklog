/*
 * Copyright (c) 2014 Jozsef Galajda <jgalajda@pannongsm.hu>
 * All rights reserved.
 */

#ifndef __QLOG_EXT_H
#define __QLOG_EXT_H

typedef enum {
    QLOG_EXT_EVENT_NONE = 0,
    QLOG_EXT_EVENT_HEXDUMP,
    QLOG_EXT_EVENT_BT,
    QLOG_EXT_EVENT_CUSTOM
} qlog_ext_event_type_t;

typedef void (*qlog_ext_print_cb_t)(FILE* stream, void* data, size_t data_size);

qlog_ext_print_cb_t qlog_ext_get_print_cb(qlog_ext_event_type_t ext_event_type);
int qlog_ext_log(qlog_ext_event_type_t event_type, void* ext_data, size_t data_size, const char* message);
int qlog_ext_log_long(qlog_ext_event_type_t event_id,
                      void* ext_data,
                      size_t data_size,
                      const char* thread,
                      const char* function_name,
                      int line_number,
                      const char* message);




#endif
