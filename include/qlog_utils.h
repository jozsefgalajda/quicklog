/*
 * Copyright (c) 2014 Jozsef Galajda <jgalajda@pannongsm.hu>
 * All rights reserved.
 */

#include <execinfo.h>

#define QLOG_VA(format_str, ...)                                \
    do {                                                        \
        char buffer[256];                                       \
        memset(buffer, 0, sizeof(buffer));                      \
        snprintf(buffer, sizeof(buffer) - 1,                    \
                 format_str, ## __VA_ARGS__);                   \
        qlog_log_long(NULL, __FUNCTION__, __LINE__, buffer);    \
    } while (0);

#define QLOG(message)                                           \
    do {                                                        \
        qlog_log_long(NULL, __FUNCTION__, __LINE__, message);   \
    } while (0);


#define QLOG_ENTRY                                              \
    do {                                                        \
        qlog_inc_indent();                                      \
        qlog_log_long(NULL, __FUNCTION__, __LINE__, "ENTRY");   \
    } while (0);

#define QLOG_LEAVE                                              \
    do {                                                        \
        qlog_log_long(NULL, __FUNCTION__, __LINE__, "LEAVE");   \
        qlog_dec_indent();                                      \
    } while (0);

#define QLOG_RET_FN(function, ret_type)                         \
    do {                                                        \
        ret_type temp_ret_value;                                \
        temp_ret_value = function;                              \
        qlog_log_long(NULL, __FUNCTION__, __LINE__, "LEAVE");   \
        qlog_dec_indent();                                      \
        return temp_ret_value;                                  \
    } while (0);

#define QLOG_RET_EXP(expression)                                \
    do {                                                        \
        qlog_log_long(NULL, __FUNCTION__, __LINE__, "LEAVE");   \
        qlog_dec_indent();                                      \
        return (expression);                                    \
    } while (0);

#define QLOG_BT                                                 \
    do {                                                        \
        void* array[256];                                       \
        size_t size = 0;                                        \
        memset(array, 0 ,sizeof(array));                        \
        size = backtrace(array, sizeof(array));                 \
        qlog_ext_log_long(QLOG_EXT_EVENT_TYPE_BT,               \
                          array, size * sizeof(void*),          \
                          NULL, __FUNCTION__,                   \
                          __LINE__, "Backtrace");               \
    } while (0);

#define QLOG_HEX(data, data_size)                               \
    do {                                                        \
        qlog_ext_log_long(QLOG_EXT_EVENT_TYPE_HEXDUMP,          \
                          data, data_size, NULL,                \
                          __FUNCTION__, __LINE__,               \
                          "External log message");              \
    } while (0);

#define QLOG_INC_IND                                            \
    do {                                                        \
        qlog_inc_indent();                                      \
    } while (0);

#define QLOG_DEC_IND                                            \
    do {                                                        \
        qlog_dec_indent();                                      \
    } while (0);


