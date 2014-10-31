/*
 * Copyright (c) 2014 Jozsef Galajda <jgalajda@pannongsm.hu>
 * All rights reserved.
 */

/**
 * \file qlog.h
 * \brief Public data structures and funtion definitions.
 *
 * The qlog library can be used to store internal log information
 * which could be helpful during software development. This should not be
 * used in production code.
 *
 * The messages can be displayed by connecting to the qlog tcp server started
 * in the qlog process by telnet.
 */
#ifndef __QLOG_H
#define __QLOG_H

typedef unsigned char qlog_buffer_id_t;

#define QLOG_RET_OK             0
#define QLOG_RET_ERR            -1
#define QLOG_RET_EVNT_LOCKED    -2
#define QLOG_RET_ALREADY_INITED -3

int qlog_init(size_t size);
void qlog_thread_init(const char* thread_name);
int qlog_reset(void);
int qlog_reset_buffer_id(qlog_buffer_id_t buffer_id);
void qlog_cleanup(void);
qlog_buffer_id_t qlog_create_buffer(size_t size);
int qlog_delete_buffer(qlog_buffer_id_t buffer_id);
int qlog_log(const char* message);
int qlog_log_id(qlog_buffer_id_t buffer_id, const char* message);
int qlog_log_long(const char* thread, const char* function, unsigned int line_num, const char* message);
int qlog_log_long_id(qlog_buffer_id_t buffer_id, const char* thread, const char* function, unsigned int line_num, const char* message);
void qlog_toggle_status(void);
int qlog_get_status(void);

int qlog_start_server(void);
void qlog_wait_for_server(void);

#endif
