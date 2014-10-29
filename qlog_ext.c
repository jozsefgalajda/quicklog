#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <execinfo.h>
#include "qlog.h"
#include "qlog_internal.h"


extern int qlog_lib_inited;
extern qlog_buffer_t* qlog_default_buf;
extern int qlog_enabled;
extern __thread char qlog_thread_name[16];


void qlog_ext_display_hex_dump(FILE* stream, void* datap, size_t size){
    int i = 0, j = 0; 
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


void qlog_ext_display_generic(FILE* stream, void* data, size_t size){
    fprintf(stream, "\tBacktrace:\n%s\n", (char*)data);
}


char* qlog_ext_generate_bt(void){
    void *array[256];
    size_t size, i, len;
    char **strings;
    size_t total_buffer_size = 0;
    char *result = NULL;

    size = backtrace(array, sizeof(array));
    strings = backtrace_symbols(array, size);

    /* Count the total buffer space needed to store the bt string.
     * sum the length of the strings plus add the extra buffer
     * space needed for "frame #x: " and the new line and the ending \0*/
    for (i = 0; i < size; i++){
        total_buffer_size += strlen(strings[i]);
    }
    total_buffer_size += size * 13 + 1;

    result = (char*)malloc(total_buffer_size);
    memset(result, 0, total_buffer_size);

    for (i = 0; i < size; i++){
        len = strlen(result);
        snprintf(result + len, total_buffer_size - len, "\tframe #%-3lu: %s\n", i, strings[i]);
    }

    free (strings);
    return result;
}

int qlog_ext_log(qlog_ext_event_type_t event_type, void* ext_data, size_t data_size, const char* message){
    int res = QLOG_RET_ERR;
    char* thread_name = NULL;
    if (qlog_lib_inited && qlog_default_buf && qlog_enabled && 
            event_type != QLOG_EXT_EVENT_NONE) {
        if (qlog_thread_name[0] != '0'){
            thread_name = qlog_thread_name;
        }
        if (event_type == QLOG_EXT_EVENT_BT){
            char * bt = qlog_ext_generate_bt();
            qlog_log_internal(qlog_default_buf, thread_name, NULL, 0, message, bt, strlen(bt) + 1,  event_type);
            free(bt);
        } else {
            res = qlog_log_internal(qlog_default_buf, thread_name, NULL, 0, message, ext_data, data_size, event_type);
        }
    }
    return res;
}

int qlog_ext_log_id(qlog_buffer_id_t buffer_id, qlog_ext_event_type_t event_type, void* ext_data, size_t data_size){
    return QLOG_RET_ERR;
}

int qlog_ext_log_long(qlog_ext_event_type_t event_type,
        void* ext_data,
        size_t data_size,
        const char* thread,
        const char* function_name,
        int line_number,
        const char* message)
{
    int res = QLOG_RET_ERR;
    char* thread_name = NULL;
    if (qlog_lib_inited && qlog_default_buf && qlog_enabled && event_type != QLOG_EXT_EVENT_NONE) {
        if (thread == NULL && qlog_thread_name[0] != '0'){
            thread_name = qlog_thread_name;
        } 
        if (event_type == QLOG_EXT_EVENT_BT) {
            char * bt = qlog_ext_generate_bt();
            qlog_log_internal(qlog_default_buf, thread_name, function_name, line_number, message, bt, strlen(bt) + 1,  event_type);
            free(bt);
        } else {
            res = qlog_log_internal(qlog_default_buf, thread_name, function_name, line_number, message, ext_data, data_size, event_type);
        }
    }
    return res;
}


int qlog_ext_log_long_id(qlog_buffer_id_t buffer_id, 
        qlog_ext_event_type_t event_id,
        void* ext_data,
        size_t data_size,
        char* thread_name,
        char* function_name,
        int line_number)
{
    return QLOG_RET_ERR;
}

qlog_ext_print_cb_t qlog_ext_get_print_cb(qlog_ext_event_type_t ext_event_type){
    qlog_ext_print_cb_t fn = NULL;
    switch (ext_event_type){
        case QLOG_EXT_EVENT_HEXDUMP:
            fn = qlog_ext_display_hex_dump;
            break;
        case QLOG_EXT_EVENT_BT:
            fn = qlog_ext_display_generic;
            break;
        case QLOG_EXT_EVENT_CUSTOM:
            break;
        default:
            break;
    }
    return fn;
}
