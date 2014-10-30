/*
 * Copyright (c) 2014 Jozsef Galajda <jgalajda@pannongsm.hu>
 * All rights reserved.
 */

/**
 * \file qlog.c
 * \brief qlog library implementation
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdint.h>

#include "qlog.h"
#include "qlog_internal.h"
#include "qlog_debug.h"
#include "qlog_display.h"

qlog_buffer_t* qlog_default_buf = 0;
int qlog_lib_inited = 0;
int qlog_enabled = 0;
qlog_buffer_t* qlog_buffers[QLOG_MAX_BUF_NUM];
pthread_spinlock_t qlog_global_lock;
static qlog_lock_state_t qlog_global_lock_state = QLOG_LOCK_UNINITED;
__thread char qlog_thread_name[16] = {0};

/******************************************************************************
 *
 * P U B L I C  functions
 *
 ******************************************************************************/


/**
 * \brief Initializes the qlog library. Allocates the default log buffer.
 *
 * \param size The maximum number of log messages in the log buffer. 
 *             If 0, no default buffer is allocated
 * \return 0 on success, -1 in case of error
 *
 * Initializes the qlog logging library.
 * If the size parameter is 0, we use the default buffer size.
 */
int qlog_init(size_t size){
    int spin_res = 0;
    if (qlog_lib_inited == 1) {
        return QLOG_RET_ALREADY_INITED;
    }

    if (size > QLOG_MAX_EVENT_NUM) {
        size = QLOG_MAX_EVENT_NUM;
    }

    /* init the global lock */
    spin_res = pthread_spin_init(&qlog_global_lock, PTHREAD_PROCESS_PRIVATE);
    if (spin_res) {
        return QLOG_RET_ERR;
    }
    qlog_global_lock_state = QLOG_LOCK_UNLOCKED;

    /* set up the buffer pointer array and allocate the default
     * log buffer
     */
    memset(qlog_buffers, 0, sizeof(qlog_buffers));

    if (size != 0) {
        qlog_buffers[0] = qlog_init_buffer_internal(size);
        if (qlog_buffers[0]) {
            qlog_default_buf = qlog_buffers[0];
        }
    }
    qlog_enable_internal();
    qlog_lib_inited = 1;

    return QLOG_RET_OK;
}

/**
 * \brief Sets the thread name TLS variable
 *
 * \param thread_name The name of the thread
 *
 * If this variable is set, the events logged by the thread
 * will contain the thread name
 */
void qlog_thread_init(const char* thread_name){
    if (thread_name){
        snprintf(qlog_thread_name, sizeof(qlog_thread_name) - 1, "%s", thread_name);
    }
}

/**
 * \brief Resets ALL qlog log buffer - clean sheet
 *
 * \return QLOG_RET_OK on success. QLOG_RET_ERR in case of any error
 *
 * Resets all log buffers, cleans the messages. After calling this the
 * buffers will contain no messages.
 */
int qlog_reset(void){
    int res = QLOG_RET_ERR;
    int lock_res = 0;
    int i = 0;

    if (qlog_lib_inited) {
        lock_res = qlog_lock_global(0);
        if (lock_res != QLOG_RET_OK){
            return lock_res;
        }
        for (i = 0; i < QLOG_MAX_BUF_NUM; i++){
            if (qlog_buffers[i]){
                res = qlog_reset_buffer_internal(qlog_buffers[i]);
                if (res != QLOG_RET_OK){
                    break;
                }
            }
        }
    }
    lock_res = qlog_unlock_global();
    return (res == QLOG_RET_OK && lock_res == QLOG_RET_OK) ? QLOG_RET_OK : QLOG_RET_ERR;
}

/**
 * \brief Resets the specified qlog log buffer
 *
 * \param buffer_id the id of the log buffer to be reset
 * \return QLOG_RET_OK on success. QLOG_RET_ERR in case of any error
 *
 * Resets the selected log buffer, cleans the messages. After calling this the
 * buffer will contain no messages.
 */
int qlog_reset_buffer_id(qlog_buffer_id_t buffer_id){
    int res = QLOG_RET_ERR;
    if (qlog_lib_inited){
        if (buffer_id < QLOG_MAX_BUF_NUM && qlog_buffers[buffer_id]){
            res = qlog_reset_buffer_internal(qlog_buffers[buffer_id]);
        }
    }
    return res;
}


/**
 * \brief Library cleanup function
 *
 * Generic cleanup routine. Frees all allocated memory (events and the buffers).
 * After calling this function no new logs are accepted.
 */
void qlog_cleanup(void){
    int i = 0, lock_res = 0;
    if (qlog_lib_inited){
        lock_res = qlog_lock_global(0);
        if (lock_res != QLOG_RET_OK){
            return;
        }
        qlog_disable_internal();

        /* for all buffers call the internal cleanup routine */
        for (i = 0; i < QLOG_MAX_BUF_NUM; i++){
            if (qlog_buffers[i]){
                qlog_cleanup_buffer_internal(qlog_buffers[i]);
            }
        }

        /* delete all buffer pointers, invalidate the
         * default buffer pointer, release and destroy the global lock */
        qlog_lib_inited = 0;
        qlog_default_buf = NULL;
        memset(qlog_buffers, 0, sizeof(qlog_buffers));

        lock_res = pthread_spin_unlock(&qlog_global_lock);
        if (lock_res){
            return;
        }
        lock_res = pthread_spin_destroy(&qlog_global_lock);
        qlog_global_lock_state = QLOG_LOCK_UNINITED;
    }
}

/**
 * \brief Create a new qlog log buffer
 *
 * \param size The maximum number of log messages in the log buffer.
 * \return the index of the new buffer or -1 in case of any error
 *
 * Searches for a free buffer and initializes it.
 * Returns with the index of the new buffer. This ID has to be used
 * as a parameter of the qlog_*_id functions to select the buffer to
 * place the log message into.
 */
qlog_buffer_id_t qlog_create_buffer(size_t size){
    int buffer_index = -1;
    int i = 0;
    int lock_res = QLOG_RET_ERR;

    if (qlog_lib_inited){
        lock_res = qlog_lock_global(0);
        if (lock_res != QLOG_RET_OK){
            return -1;
        }

        /* search for the first free buffer id */
        for (i = 0; i < QLOG_MAX_BUF_NUM; i++){
            if (qlog_buffers[i] == NULL){
                buffer_index = i;
                break;
            }
        }

        /* there is a free buffer, initialize it */
        if (buffer_index >= 0) {    
            if (size == 0 || size > QLOG_MAX_EVENT_NUM) {
                size = QLOG_MAX_EVENT_NUM;
            }
            qlog_buffers[buffer_index] = qlog_init_buffer_internal(size);
            if (qlog_default_buf == NULL){
                qlog_default_buf = qlog_buffers[buffer_index];
            }
        }

        lock_res = qlog_unlock_global();
        if ( buffer_index >= 0 && qlog_buffers[buffer_index] != NULL && lock_res == QLOG_RET_OK){
            return buffer_index;
        }
    }
    return -1;
}

/**
 * \brief Logs a new event to the default log buffer
 *
 * \param message The log message string
 * \return 0 on success, -1 in case of error
 *
 * Creates a new log message in the buffer. This is the short version where
 * only the message is used. All the other parameters will be empty.
 */
int qlog_log(const char* message){
    int res = QLOG_RET_ERR;
    char* thread_name = NULL;
    if (qlog_lib_inited && qlog_default_buf && qlog_enabled && message) {
        if (qlog_thread_name[0] != '0'){
            thread_name = qlog_thread_name;
        }
        res = qlog_log_internal(qlog_default_buf, thread_name, NULL, 0, message, NULL, 0, QLOG_EXT_EVENT_NONE);
    }
    return res;
}

/**
 * \brief Logs a new event to the log buffer with the specified id.
 *
 * \param buffer_id The id of the buffer into the message will be placed
 * \param message The log message string
 * \return 0 on success, -1 in case of error
 *
 * Creates a new log message in the buffer with buffer_id. 
 * The buffer has to be created with qlog_create_buffer().
 * This is the short version where only the message is used. 
 * All the other parameters will be empty.
 */
int qlog_log_id(qlog_buffer_id_t buffer_id, const char* message){
    int res = QLOG_RET_ERR;
    if (qlog_lib_inited && qlog_enabled && message){
        if (buffer_id < QLOG_MAX_BUF_NUM && qlog_buffers[buffer_id]){
            res = qlog_log_internal(qlog_buffers[buffer_id], NULL, NULL, 0, message, NULL, 0, QLOG_EXT_EVENT_NONE);
        }
    }
    return res;
}

/**
 * \brief Logs a new event with all the possible parameters to the default log buffer
 *
 * \param thread The name of the thread generating the log message. (optional)
 * \param function The name of the function generating the log message (optional)
 * \param line_num The line number in the source file of the log message (optional)
 * \param message The log message string
 * \return 0 if success, -1 in case of any error
 *
 * Creates a new log event in the buffer. This is the longest version where all the
 * possible parameters can be provided. The message parameter has to be valid
 * (not NULL), any of the rest can be NULL or 0 in case it is not needed in the
 * message.
 */
int qlog_log_long(const char* thread, 
        const char* function, 
        unsigned int line_num, 
        const char* message)
{
    int res = QLOG_RET_ERR;
    char * thread_name = NULL;
    if (qlog_lib_inited && qlog_default_buf && qlog_enabled && message) {
        if (thread == NULL && qlog_thread_name[0] != '\0'){
            thread_name = qlog_thread_name;
        }
        res = qlog_log_internal(qlog_default_buf, thread_name, function, line_num, message, NULL, 0, QLOG_EXT_EVENT_NONE);
    }
    return res;
}

/**
 * \brief Logs a new event with all the possible parameters to a buffer with a specified id
 *
 * \param buffer_id the id of the buffer into the message will be put
 * \param thread The name of the thread generating the log message. (optional)
 * \param function The name of the function generating the log message (optional)
 * \param line_num The line number in the source file of the log message (optional)
 * \param message The log message string
 * \return 0 if success, -1 in case of any error
 *
 * Creates a new log event in the buffer with id of buffer_id. 
 * The buffer has to be created first  with qlog_create_buffer(). 
 * This is the longest version where all the possible parameters can be 
 * provided. The message and buffer_id parameters have to be 
 * valid, any of the rest can be NULL or 0 in case it is not needed in the
 * message.
 */
int qlog_log_long_id(
        qlog_buffer_id_t buffer_id, 
        const char* thread, 
        const char* function, 
        unsigned int line_num, 
        const char* message)
{    
    int res = QLOG_RET_ERR;
    if (qlog_lib_inited && qlog_enabled && message){
        if (buffer_id < QLOG_MAX_BUF_NUM && qlog_buffers[buffer_id]){
            res = qlog_log_internal(qlog_buffers[buffer_id], thread, function, line_num, message, NULL, 0, QLOG_EXT_EVENT_NONE);
        }
    }
    return res;
}

/******************************************************************************
 *
 * P R I V A T E  functions
 *
 ******************************************************************************/

/**
 * \brief Internal buffer initialization function
 *
 * \param size The maximum number of log messages in the log buffer.
 * \return Pointer to the allocated log buffer or NULL in case of error.
 *
 * By design the library is capable of creating and using multiple log buffers.
 * For the first time only one default log buffer will be used/available for the library
 * user, and the user does not have to provide the log buffer when using the libtrary.
 * The internal functions are capable of working with log buffers other than the default one.
 * Because of this all public library function is a wrapper in which 
 * the internal function is called with the default log buffer.
 */
qlog_buffer_t* qlog_init_buffer_internal(size_t size){
    qlog_buffer_t* buffer = 0;
    qlog_event_t* event = NULL, *prev_event = NULL;
    unsigned int i = 0;
    int res = 0;

    /* Allocate the log buffer */
    buffer = (qlog_buffer_t*) malloc(sizeof(qlog_buffer_t));
    if (buffer == NULL){
        return NULL;
    }
    memset(buffer, 0, sizeof(qlog_buffer_t));

    /* Allocate all log event structures */
    for (i = 0; i < size; i++){
        event = (qlog_event_t*) malloc(sizeof(qlog_event_t));
        if (event == NULL) {
            qlog_cleanup_buffer_internal(buffer);
            return NULL;
        }
        memset(event, 0, sizeof(qlog_event_t));
        if (buffer->head == NULL) {
            buffer->head = event;
        } else {
            prev_event->next = event;
        }
        prev_event = event;
    }

    /* Set the defaults, initialize lock */
    event->next = buffer->head;
    buffer->next_write = buffer->head;
    buffer->buffer_size = size;
    res = pthread_spin_init(&buffer->lock, PTHREAD_PROCESS_PRIVATE);
    if (res) {
        qlog_cleanup_buffer_internal(buffer);
        return NULL;
    }

    return buffer;
}


/**
 * \brief Internal library reset function
 *
 * \param log_buffer The log buffer to be reset
 * \return 0 on success, -1 in case of any error
 *
 * Resets the log buffer provided as a parameter.
 */
int qlog_reset_buffer_internal(qlog_buffer_t* log_buffer) {
    int res = QLOG_RET_ERR, start = 1;
    qlog_event_t *event = NULL;

    if (log_buffer){
        res = qlog_lock_buffer_internal(log_buffer);
        if (res){
            return res;
        }

        event = log_buffer->head;
        while (event){
            if (event == log_buffer->head){
                if (start == 1){    /* check if we have reached back to the head again or just starting to delete*/
                    start = 0;
                    qlog_reset_event_internal(event);
                    event = event->next;
                } else {
                    event = NULL;
                }
            } else {
                qlog_reset_event_internal(event);
                event = event->next;
            }
        }
        log_buffer->wrapped = 0;
        log_buffer->event_locked = 0;
        res = qlog_unlock_buffer_internal(log_buffer);
    }
    return res;
}

/**
 * \brief Internal event reset function
 *
 * \param event The event to be cleared/reset
 *
 * Deletes all the fields in the event structure provided as a parameter
 */
void qlog_reset_event_internal(qlog_event_t* event){
    if (event){
        memset(event->function_name, 0, QLOG_FNAME_BUF_SIZE);
        memset(event->thread_name, 0, QLOG_TNAME_BUF_SIZE);
        memset(event->message, 0, QLOG_MSG_BUF_SIZE);
        event->line_number = 0;
        memset(&event->timestamp, 0, sizeof(struct timeval));
        event->lock = 0;
        event->used = 0;
        if (event->ext_data){
            free(event->ext_data);
            event->ext_data = NULL;
        }
        event->ext_data_size = 0;
        event->ext_event_type = QLOG_EXT_EVENT_NONE;
        event->ext_print_cb = NULL;
    }
}

/**
 * \brief Internal event cleanup function
 *
 * \param event The event to be cleaned up
 *
 * Generic event cleanup routine. 
 * Free the extended data if allocated for the event.
 */
void qlog_cleanup_event_internal(qlog_event_t* event){
    if (event){
        if (event->ext_data){
            free(event->ext_data);
            event->ext_data = NULL;
        }
        free(event);
    }
}


/**
 * \brief Internal library cleanup function
 *
 * \param buffer The qlog buffer to be released
 *
 * Generic cleanup routine. Free all allocated memory (events and the buffer)
 */
/* TODO: free spinlock of the buffer */
void qlog_cleanup_buffer_internal(qlog_buffer_t* buffer){
    qlog_event_t *event, *tmp = 0;
    int res = 0;
    int start = 1;
    if (buffer){
        res = qlog_lock_buffer_internal(buffer); /* lock the buffer so no other thread will try to log a new event */
        if (res == -1) {
            return;
        }
        event = buffer->head;
        while (event){
            tmp = event;
            if (event == buffer->head){
                if (start == 1){    /* check if we have reached back to the head again or just starting to delete*/
                    start = 0;
                    event = event->next;
                    qlog_cleanup_event_internal(tmp);
                } else {
                    event = NULL;
                }
            } else {
                event = event->next;
                qlog_cleanup_event_internal(tmp);
            }
        }
        free(buffer);
    }
}

/**
 * \brief Internal function for saving a new log message in the buffer
 *
 * \param log_buffer The buffer into the new message will be placed
 * \param thread The thread name from where the message is logged (optional)
 * \param function The name of the function from where the message is logged (optional)
 * \param line_num The source code line number of the log message (optional)
 * \param message The log message string
 * \return 0 on success, -1 in case of error
 *
 * This function is the workhorse of the log message handling.
 * Gets the next free log buffer position and copies the message into the buffer
 * The buffer is locked only for the pointer handling. As soon as the log message
 * structure for the new message is secured, a temporary pointer is provided, and the 
 * buffer lock is released.
 */
int qlog_log_internal(
        qlog_buffer_t* log_buffer, 
        const char* thread, 
        const char* function, 
        unsigned int line_num, 
        const char* message,
        void* ext_data,
        size_t ext_data_size,
        qlog_ext_event_type_t ext_event_type)
{
    qlog_event_t* event = NULL;
    int res = 0;

    /* grab the buffer lock
     * get the next free slot and release the lock as soon as possible 
     * if the lock cannot be aquired, return with error
     */
    res = qlog_lock_buffer_internal(log_buffer);
    if (res == 0){
        event = log_buffer -> next_write;
        log_buffer -> next_write = event->next;
        if (log_buffer->next_write == log_buffer->head){
            log_buffer->wrapped++;
        }
        res = qlog_unlock_buffer_internal(log_buffer);
        if (res) {
            return QLOG_RET_ERR;
        }
    } else {
        return QLOG_RET_ERR;
    }

    /* Check if the event is not locked, i.e. other thread is not
     * filling the event structure. This could happen if the buffer
     * is wrapped since the event structure is provided to another
     * thread but that one has not been finished updating the event 
     * structure. 
     *
     * The buffer lock is placed to the buffer only during the
     * next write pointer is selected. If the buffer wraps very 
     * often, the same events could be used by multiple threads
     * so we have to lock the event structure as well.
     *
     * After the event is filled with the data, we release the 
     * event structure lock.
     *
     * If we happen to get a event which is currently locked,
     * we return with -1 and do not store the event.
     */
    if (event->lock == 0) {
        __sync_fetch_and_add(&event->lock, 1);
    } else {
        __sync_fetch_and_add(&log_buffer->event_locked, 1);
        return QLOG_RET_EVNT_LOCKED;
    }

    gettimeofday(&(event->timestamp), NULL);

    event->thread_name[0] = '\0';
    event->function_name[0] = '\0';
    event->message[0] = '\0';
    /* store thread name if it is provided */
    if (thread){
        strncpy(event->thread_name, thread, QLOG_TNAME_BUF_SIZE - 1);
    }

    /* store the function name if it is provided */
    if (function){
        strncpy(event->function_name, function, QLOG_FNAME_BUF_SIZE - 1);
    }

    /* store or clear the line number */
    event->line_number = line_num;

    /* store the log message */
    if (message) {
        strncpy(event->message, message, QLOG_MSG_BUF_SIZE - 1);
    }

    /* clean up external event data */
    if (event->ext_data != NULL){
        free(event->ext_data);
        event->ext_data = NULL;
        event->ext_data_size = 0;
        event->ext_event_type = QLOG_EXT_EVENT_NONE;
    }

    /* if external log data has been provided, store it in the event */
    if (ext_event_type != QLOG_EXT_EVENT_NONE && ext_data && ext_data_size > 0) {
        event->ext_data = malloc(ext_data_size);
        memcpy(event->ext_data, ext_data, ext_data_size);
        event->ext_event_type = ext_event_type;
        event->ext_print_cb = qlog_ext_get_print_cb(ext_event_type);
        event->ext_data_size = ext_data_size;
    }

    event->used = 1;
    __sync_fetch_and_sub(&event->lock, 1);

    return QLOG_RET_OK;
}


/**
 * \brief Internal buffer locking function. Aquire buffer lock.
 *
 * \param buffer Pointer to the buffer to be locked
 * \return 0 in case the lock is aquired, -1 otherwise.
 *
 * Tries to lock the buffer. It the buffer is locked by another thread,
 * the block parameter determines how to proceed. If the parameter is >0,
 * we wait until the lock gets free and we can get it. If the parameter is 0,
 * we do not wait for the lock.
 */
int qlog_lock_buffer_internal(qlog_buffer_t* buffer){
    int res = 0;
    if (qlog_lib_inited && buffer) {
        res = pthread_spin_lock(&buffer->lock);
        return res  == 0 ? QLOG_RET_OK : QLOG_RET_ERR;
    }
    return QLOG_RET_ERR;
}

/**
 * \brief Internal buffer lock release function. Releases buffer lock
 *
 * \param buffer Pointer to the buffer to be released 
 * \return 0 in case the lock is aquired, -1 otherwise.
 */
int qlog_unlock_buffer_internal(qlog_buffer_t* buffer) {
    int res = 0;
    if (qlog_lib_inited && buffer){
        res = pthread_spin_unlock(&buffer->lock);
        return res == 0 ? QLOG_RET_OK : QLOG_RET_ERR;
    }
    return QLOG_RET_ERR;
}

/**
 * \brief Disable the logging
 *
 * Disables the logging globally.
 */
void qlog_disable_internal(void){
    if (qlog_enabled == 1){
        __sync_fetch_and_sub(&qlog_enabled, 1);
    }
}

/**
 * \brief Enable the logging
 *
 * Disables the logging globally.
 */
void qlog_enable_internal(void){
    if (qlog_enabled == 0){
        __sync_fetch_and_add(&qlog_enabled, 1);
    }
}

/**
 * \brief Global locking function
 * 
 * \param full_lock (flag) if not 0 lock all buffers as well and disable logging
 * \return QLOG_RET_OK if all the locking operation were successful,
 *         QLOG_RET_ERR otherwise.
 *
 * Grabs the global lock. If the flag is not zero disables the logging and grabs
 * all buffer locks as well. The logging remains blocked until the
 * qlog_unlock_global is not called.
 * In case of any problems no cleanup is performed as we do not know
 * the status of the library/buffers.
 */
int qlog_lock_global(int full_lock){
    int res = QLOG_RET_ERR;
    int pt_res = 0;
    int i = 0;

    if (qlog_lib_inited && qlog_global_lock_state == QLOG_LOCK_UNLOCKED){
        /* grab the global lock */
        pt_res = pthread_spin_lock(&qlog_global_lock);
        if (pt_res){
            return QLOG_RET_ERR;
        }
        qlog_global_lock_state = QLOG_LOCK_SIMPLE;

        if (full_lock) {
            /* disable the logging globally */
            qlog_disable_internal();

            /* lock all the buffers individially */
            for (i = 0; i < QLOG_MAX_BUF_NUM; i++) {
                if (qlog_buffers[i]) {
                    qlog_lock_buffer_internal(qlog_buffers[i]);
                    if (res != QLOG_RET_OK) {
                        return res;
                    }
                }
            }
            qlog_global_lock_state = QLOG_LOCK_FULL;
        }
        return QLOG_RET_OK;
    }
    return QLOG_RET_ERR;
}

/**
 * \brief Global unlocking function
 *
 * \return QLOG_RET_OK if all the unlocking operation were successful,
 *         QLOG_RET_ERR otherwise.
 *
 * Releases the global lock, releases all the buffer locks and enables logging.
 * In case of any problems no cleanup is performed as we do not know
 * the status of the library/buffers.
 */
int qlog_unlock_global(void){
    int i = 0;
    int pt_res = 0;
    int res = QLOG_RET_ERR;

    if (qlog_lib_inited && (qlog_global_lock_state == QLOG_LOCK_SIMPLE || qlog_global_lock_state == QLOG_LOCK_FULL)){
        if (qlog_global_lock_state == QLOG_LOCK_FULL) {
            for (i = 0; i < QLOG_MAX_BUF_NUM; i++){
                if (qlog_buffers[i]){
                    res = qlog_unlock_buffer_internal(qlog_buffers[i]);
                    if (res != QLOG_RET_OK){
                        return res;
                    }
                }
            }
        }

        pt_res = pthread_spin_unlock(&qlog_global_lock);
        if (pt_res) {
            return QLOG_RET_ERR;
        }

        if (qlog_global_lock_state == QLOG_LOCK_FULL){
            qlog_enable_internal();
        }
        qlog_global_lock_state = QLOG_LOCK_UNLOCKED;

        return QLOG_RET_OK;
    }
    return QLOG_RET_ERR;
}

/**
 * \brief Provides the buffer pointer by buffer id
 *
 * \param buffer_id The id of the log buffer
 * 
 * Returns with the buffer pointer by the specified id value
 */
qlog_buffer_t* qlog_internal_get_buffer_by_id(qlog_buffer_id_t buffer_id){
    if (qlog_lib_inited && buffer_id < QLOG_MAX_BUF_NUM){
        return qlog_buffers[buffer_id];
    }
    return NULL;
}

int qlog_internal_get_max_buf_num(void){
    return QLOG_MAX_BUF_NUM;
}

/******************************************************************************
 *
 * I N T E R N A L  D E B U G  functions
 *
 ******************************************************************************/

qlog_buffer_t* qlog_dbg_get_default_buffer(void){
    return qlog_default_buf;
}

void qlog_dbg_print_event(FILE* stream, qlog_event_t* event){
    char buffer[256];
    qlog_display_format_event_str(event, buffer, sizeof(buffer));
    if (buffer[0] != '\0'){
        fprintf(stream, "%s\n", buffer);
    }
}

void qlog_dbg_print_buffer(FILE* stream, qlog_buffer_t* buffer){
    int res = 0;
    int start = 1;
    qlog_event_t* event = NULL;

    if (buffer){
        res = qlog_lock_buffer_internal(buffer);
        if (res){
            return;
        }
        fprintf(stream, "--------------------------------------------------\n");
        fprintf(stream, " Buffer head         : %p\n", (void*) buffer->head);
        fprintf(stream, " Buffer next         : %p\n", (void*) buffer->next_write);
        fprintf(stream, " Buffer size         : %u\n", (unsigned int) buffer->buffer_size);
        fprintf(stream, " Buffer wrapped      : %d\n", buffer->wrapped);
        fprintf(stream, " Buffer event locked : %d\n", buffer->event_locked);
        fprintf(stream, "--------------------------------------------------\n");
        event = buffer->head;
        while(event){
            if (event == buffer->head){
                if (start == 1){
                    start = 0;
                    qlog_dbg_print_event(stream, event);
                    event = event->next;
                } else {
                    event = NULL;
                }
            } else {
                qlog_dbg_print_event(stream, event);
                event = event->next;
            }
        }

        res = qlog_unlock_buffer_internal(buffer);
    } else {
        fprintf(stream, "The buffer is not initialized\n");
    }
}


void qlog_dbg_print_buffers(FILE* stream){
    int i = 0;
    int start = 1;
    qlog_event_t* event = NULL;
    if (qlog_lib_inited){
        pthread_spin_lock(&qlog_global_lock);
        fprintf(stream, "==================================================\n");
        fprintf(stream, " QLOG buffers\n");
        fprintf(stream, "==================================================\n");
        for (i = 0; i < QLOG_MAX_BUF_NUM; i++){
            fprintf(stream, " Buffer index: %d\n", i);
            if (qlog_buffers[i] == NULL){
                fprintf(stream, "     This buffer is not initialized.\n");
            } else {
                qlog_lock_buffer_internal(qlog_buffers[i]);
                fprintf(stream, "     Buffer head         : %p\n", (void*) qlog_buffers[i]->head);
                fprintf(stream, "     Buffer next         : %p\n", (void*) qlog_buffers[i]->next_write);
                fprintf(stream, "     Buffer size         : %u\n", (unsigned int) qlog_buffers[i]->buffer_size);
                fprintf(stream, "     Buffer wrapped      : %d\n", qlog_buffers[i]->wrapped);
                fprintf(stream, "     Buffer event locked : %d\n", qlog_buffers[i]->event_locked);
                fprintf(stream, "--------------------------------------------------\n");
                fprintf(stream, " Contents of this buffer\n");
                fprintf(stream, "--------------------------------------------------\n");
                event = qlog_buffers[i]->head;
                start = 1;
                while(event){
                    if (event == qlog_buffers[i]->head){
                        if (start == 1){
                            start = 0;
                            qlog_dbg_print_event(stream, event);
                            event = event->next;
                        } else {
                            event = NULL;
                        }
                    } else {
                        qlog_dbg_print_event(stream, event);
                        event = event->next;
                    }
                }
                qlog_unlock_buffer_internal(qlog_buffers[i]);
            }
            fprintf(stream, "--------------------------------------------------\n");
        }
        pthread_spin_unlock(&qlog_global_lock);
    } else {
        fprintf(stream, "The qlog library has not been initialized\n");
    }
}
