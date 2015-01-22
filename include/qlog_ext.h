/*
 * Copyright (c) 2014 Jozsef Galajda <jgalajda@pannongsm.hu>
 * All rights reserved.
 */

#ifndef __QLOG_EXT_H
#define __QLOG_EXT_H

typedef unsigned int qlog_ext_event_type_t;
typedef void (*qlog_ext_print_cb_t)(FILE* stream, void* data, size_t data_size);

typedef struct qlog_ext_event_info_t {
    qlog_ext_event_type_t event_type;
    qlog_ext_print_cb_t print_callback;
} qlog_ext_event_info_t;

/*
 * - the built-in predefined external events have fix event type value.
 * - the dynamiclaly defined external events will have an event type value
 *   assigned from QLOG_EXT_EVENT_TYPE_DYNAMIC_START
 * - New built-in external events have to get event type
 *   value from 0 to QLOG_EXT_EVENT_TYPE_DYNAMIC_START - 1
 */

#define QLOG_EXT_EVENT_TYPE_NONE            0
#define QLOG_EXT_EVENT_TYPE_BT              1
#define QLOG_EXT_EVENT_TYPE_HEXDUMP         2
#define QLOG_EXT_EVENT_TYPE_LAST QLOG_EXT_EVENT_TYPE_HEXDUMP
#define QLOG_EXT_EVENT_TYPE_DYNAMIC_START   100

qlog_ext_print_cb_t qlog_ext_get_print_cb(qlog_ext_event_type_t ext_event_type);

int qlog_ext_log(qlog_ext_event_type_t event_type,
        void* ext_data,
        size_t data_size,
        const char* message);

int qlog_ext_log_id(qlog_buffer_id_t buffer_id,
        qlog_ext_event_type_t event_type,
        void* ext_data,
        size_t data_size,
        const char* message);

int qlog_ext_log_long(qlog_ext_event_type_t event_id,
                      void* ext_data,
                      size_t data_size,
                      const char* thread,
                      const char* function_name,
                      int line_number,
                      const char* message);

int qlog_ext_log_long_id(qlog_buffer_id_t buffer_id,
        qlog_ext_event_type_t event_type,
        void* ext_data,
        size_t data_size,
        const char* thread_name,
        const char* function_name,
        int line_number,
        const char* message);


int qlog_ext_event_type_is_valid(qlog_ext_event_type_t event_type);

int qlog_ext_register_built_in_events(void);
void qlog_ext_display_hex_dump(FILE* stream, void* datap, size_t size);
void qlog_ext_display_bt(FILE* stream, void* datap, size_t size);
int qlog_ext_init(void);
qlog_ext_event_type_t qlog_ext_register_event(qlog_ext_print_cb_t print_callback);

#endif
