/*
 * Copyright (c) 2014 Jozsef Galajda <jgalajda@pannongsm.hu>
 * All rights reserved.
 */


#define QLOG(str)                                               \
    do {                                                        \
        qlog_log_long(NULL, __FUNCTION__, __LINE__, str);       \
    } while (0);

#define QLOG_ENTRY                                              \
    do {                                                        \
        qlog_log_long(NULL, __FUNCTION__, __LINE__, "ENTRY");   \
    } while (0);

#define QLOG_LEAVE                                              \
    do {                                                        \
        qlog_log_long(NULL, __FUNCTION__, __LINE__, "LEAVE");   \
    } while (0);

#define QLOG_BT                                                 \
    do {                                                        \
        qlog_ext_log_long(QLOG_EXT_EVENT_BT, NULL, 0, NULL, __FUNCTION__, __LINE__, "External log message");   \
    } while (0);

#define QLOG_HEX(data, data_size)   \
    do {                                                        \
        qlog_ext_log_long(QLOG_EXT_EVENT_HEXDUMP, data, data_size, NULL, __FUNCTION__, __LINE__, "External log message");   \
    } while (0);


/*
#define QLOG_VA(format_str, ...)  \
    do {    \
        char buffer[256];   \
        memset(buffer, 0, sizeof(buffer));  \
        snprintf(buffer, sizeof(buffer) - 1, format_str, ## __VA_ARGS__);    \
        qlog_log_long(NULL, __FUNCTION__, __LINE__, buffer);    \
    } while (0);
    */
