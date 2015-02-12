/*
 * Copyright (c) 2014 Jozsef Galajda <jgalajda@pannongsm.hu>
 * All rights reserved.
 */

/**
 * \file qlog_internal.h
 * \brief Internal data structures and funtion definitions.
 */
#ifndef __QLOG_INTERNAL_H
#define __QLOG_INTERNAL_H

#include "qlog_ext.h"
#define UNUSED __attribute__ ((unused))

#include <stdint.h>

#define QLOG_MAX_EVENT_NUM  128
#define QLOG_MAX_BUF_NUM    5
#define QLOG_FNAME_BUF_SIZE 32
#define QLOG_TNAME_BUF_SIZE 32
#define QLOG_MSG_BUF_SIZE   256

/**
 * \struct qlog_event_t
 * \brief Structure to hold all log event specific data.
 */
typedef struct qlog_event_t {
    char function_name[QLOG_FNAME_BUF_SIZE]; /*!< Name of the function the log comes from. Optional. */
    char thread_name[QLOG_TNAME_BUF_SIZE];   /*!< Thread name from the log comes from. Optional. */
    char message[QLOG_MSG_BUF_SIZE];         /*!< The log message itself */
    unsigned int line_number ;               /*!< The line number of the log message in the code */
    struct timeval timestamp;                /*!< Timestamp of the log message */
    struct qlog_event_t* next;               /*!< The next log event. Events are stored in linked list */
    unsigned char used;                      /*!< Event slot is free/used */
    unsigned char lock;                      /*!< The event structure is locked (getting populated with data */
    void* ext_data;                          /*!< Extended log data if log is special */
    size_t ext_data_size;                    /*!< The size of the extended log data */
    qlog_ext_event_type_t ext_event_type;    /*!< The external event type if any */
    qlog_ext_print_cb_t ext_print_cb;        /*!< Function to print/format external data to stream */
    uint8_t indent_level;                    /*!< Log message ident level */
} qlog_event_t;


/**
 * \struct qlog_buffer_t
 * \brief Structure to hold all log buffer related information
 */
typedef struct qlog_buffer_t {
    qlog_event_t* head;         /*!< Head of the event data list*/
    qlog_event_t* next_write;   /*!< The next write position in the list*/
    size_t buffer_size;         /*!< The number of events (log buffer capacity)*/
    int wrapped;                /*!< Number of buffer wraps*/
    pthread_spinlock_t lock;    /*!< Buffer lock for pointer operations */
    unsigned int event_locked;  /*!< Counter of msg drops because of event is locked */
} qlog_buffer_t;

typedef enum {
    QLOG_LOCK_UNINITED = 0, 
    QLOG_LOCK_UNLOCKED = 1,
    QLOG_LOCK_SIMPLE = 2,
    QLOG_LOCK_FULL = 3
} qlog_lock_state_t;


qlog_buffer_t* qlog_init_buffer_internal(size_t size);
int qlog_reset_buffer_internal(qlog_buffer_t* log_buffer);
void qlog_cleanup_buffer_internal(qlog_buffer_t* buffer);
void qlog_reset_event_internal(qlog_event_t* event);

int qlog_log_internal(qlog_buffer_t* log_buffer, const char* thread, 
        const char* function, unsigned int line_num, 
        const char* message, void* ext_data, size_t ext_data_size, 
        qlog_ext_event_type_t event_type);

int qlog_lock_buffer_internal(qlog_buffer_t* buffer);
int qlog_unlock_buffer_internal(qlog_buffer_t* buffer);
int qlog_lock_global(int full_lock);
int qlog_unlock_global(void);

void qlog_enable_internal(void);
void qlog_disable_internal(void);

int qlog_internal_get_max_buf_num(void);
int qlog_internal_is_lib_inited(void);
int qlog_internal_is_logging_enabled(void);
qlog_buffer_t* qlog_internal_get_default_buf(void);
qlog_buffer_id_t qlog_internal_get_default_buf_id(void);
qlog_buffer_t* qlog_internal_get_buffer_by_id(qlog_buffer_id_t buffer_id);
char* qlog_internal_get_thread_name(void);

#endif
