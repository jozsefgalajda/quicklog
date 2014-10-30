/*
 * Copyright (c) 2014 Jozsef Galajda <jgalajda@pannongsm.hu>
 * All rights reserved.
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include "qlog.h"
#include "qlog_internal.h"
#include "qlog_display.h"
#include "qlog_display_debug.h"
#include "qlog_debug.h"

pthread_t qlog_server_thread;
static int server_port = 50005;
static const char* welcome_msg = "\n  >> QuickLog log access server console <<\n\n";
static const char* qlog_server_prompt_str = "qlog> ";

static qlog_buffer_id_t active_buffer = 0;

typedef struct qlog_server_menu_item {
    const char * menu_str;
    void (*menu_function)(void);
} qlog_server_menu_item;

void qlog_menu_list_buffers(void);
void qlog_menu_print_logs(void);
void qlog_menu_stop_server(void);
void qlog_menu_close_conn(void);
void qlog_menu_error(void);
void qlog_server_print_cmd_header(FILE* stream, const char* message);
void qlog_server_print_cmd_footer(FILE* stream);

static qlog_server_menu_item qlog_server_menu[] = {
    {"[1] List log buffers", NULL},
    {"[2] Select active buffer", NULL},
    {"[3] Print logs from the active buffer",NULL}, 
    {"[4] Print logs from all buffers",NULL}, 
    {"[5] Reset (clear) the active buffer",NULL}, 
    {"[6] Reset (clear) all buffers",NULL}, 
    {"[7] Close connection\n", NULL}
};

/******************************************************************************
 * read_line
 * reads a line from the socket
 *
 * Parameters:
 * -----------
 * socket     : open socket to read from
 * buffer     : the data read is stored in this buffer
 * buffer_len : the lengh of the ourput buffer
 *
 ******************************************************************************/
int read_line(int socket, char* buffer, size_t buffer_len){
    char c;
    unsigned int chars_read = 0;
    int res = -1, eof = 0;
    if (buffer && buffer_len) {
        memset(buffer, 0, buffer_len);
        while (1) {
            res = read(socket, &c, 1);
            if (res == 1){
                if (c == '\n') {
                    break;
                } else if (c == -1) {
                    eof = 1;
                    break;
                } else if (chars_read < buffer_len - 1){
                    buffer[chars_read++] = c;
                }
            } else if (res == 0) {
                eof = 1;
                break;
            } else {
                *buffer = 0;
                break;
            }
        }
    }
    if (res < 0) {             /* error */
        return res;
    } else if (eof){           /* CTRL-C */
        return 0;
    } else {
        return chars_read;
    }
}


void qlog_server_handle_connection(int socket){
    int i = 0, j = 0;
    char buffer[8];
    int loop = 1;
    int res = 0;
    FILE* stream = NULL;
    int fd = -1;
    int max_buf_num = 0;
    char answer[8] = {0};
                
    fd = dup(socket);
    stream = fdopen(fd, "w");
                
    if (stream == NULL){
        close(fd);
        return;
    }

    max_buf_num = qlog_internal_get_max_buf_num();

    while(loop) {
        for (i = 0; i< 7; i++){
            fprintf(stream, "%s\n", qlog_server_menu[i].menu_str);
            fflush(stream);
        }
        fprintf(stream, "%s", qlog_server_prompt_str);
        fflush(stream);

        res = read_line(socket, buffer, sizeof(buffer));
        if (res <= 0) { /* error of EOF */
            loop = 0;
            break;
        }

        switch (buffer[0]) {
            case '1':
                qlog_server_print_cmd_header(stream, "List log buffers");
                fprintf(stream, "Active buffer: %d\n\n", active_buffer);
                qlog_display_print_buffer_list(stream);
                qlog_server_print_cmd_footer(stream);
                break;
            case '2':
                qlog_server_print_cmd_header(stream, "Set the active buffer");
                fprintf(stream, "Buffer id: ");
                res = read_line(socket, answer, sizeof(answer));
                if (res > 0) {
                    char* tail = NULL;
                    long int selected = 0;
                    errno = 0;
                    selected = strtol(answer, &tail, 0);
                    if (errno){
                        fprintf(stream, "Error. Fallback to the default buffer.\n");
                        active_buffer = 0;
                    } else {
                        active_buffer = (int) selected;
                        fprintf(stream, "The active buffer now is %d\n", active_buffer);
                    }
                }
                qlog_server_print_cmd_footer(stream);
                break;
            case '3':
                qlog_server_print_cmd_header(stream, "Show logs from buffer");
                fprintf(stream, "Active buffer: %d\n\n", active_buffer);
                qlog_display_print_buffer_id(stream, active_buffer);
                qlog_server_print_cmd_footer(stream);
                break;
            case '4':
                qlog_server_print_cmd_header(stream, "Show logs from all buffers");
                qlog_display_debug_print_all_buffers(stream, 1,1);
                qlog_server_print_cmd_footer(stream);
                break;
            case '5':
                qlog_server_print_cmd_header(stream, "Reset the active buffer");
                qlog_reset_buffer_id(active_buffer);
                fprintf(stream, "Active buffer: %d\n\n", active_buffer);
                fprintf(stream, "Reset has been completed.\n"); 
                qlog_server_print_cmd_footer(stream);
                break;
            case '6':
                qlog_server_print_cmd_header(stream, "Reset all log buffers");
                for (j = 0; j < max_buf_num; j++){
                    qlog_reset_buffer_id(j);
                }
                fprintf(stream, "Reset has been completed.\n"); 
                qlog_server_print_cmd_footer(stream);
                break;
            case '7':
            case 'q':
                loop = 0;
                break;
            default:
                break;
        }
    }
    if (stream){
        fclose(stream);
    }
}

void *qlog_server_handler(void* data UNUSED){
    int server_sock = 0;
    int conn_sock = 0;
    struct sockaddr_in server_addr;
    int res = 0;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0){
        fprintf(stderr, "qlog_server: Error creating server socket.\n");
        return NULL;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port        = htons(server_port);

    res = bind(server_sock, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (res < 0){
        fprintf(stderr, "qlog_server: Error binding to socket\n");
        return NULL;
    }

    res = listen(server_sock, 16);
    if (res < 0){
        fprintf(stderr, "qlog_server: Error listening on socket\n");
        return NULL;
    }

    fprintf(stderr, "qlog_server: Qlog server has been started...\n");
    while (1){
        conn_sock = accept(server_sock, NULL, NULL);
        if (conn_sock < 0){
            fprintf(stderr, "qlog_server: Error calling accept()\n");
            return NULL;
        }

        write(conn_sock, welcome_msg, strlen(welcome_msg));
        qlog_server_handle_connection(conn_sock);

        res = close(conn_sock);
        if (res < 0){
            fprintf(stderr, "qlog_server: Error closing client socket\n");
            return NULL;
        }

    }
}

int qlog_start_server(void){
    int rc = 0;
    rc = pthread_create(&qlog_server_thread, 0, qlog_server_handler, 0);
    return rc;
}

void qlog_wait_for_server(void){
    pthread_join(qlog_server_thread, 0);
}


void qlog_menu_list_buffers(void){

}
void qlog_menu_print_logs(void){}
void qlog_menu_stop_server(void){}
void qlog_menu_close_conn(void){}
void qlog_menu_error(void){}



void qlog_server_print_cmd_header(FILE* stream, const char* message){
    if (stream){
        fprintf(stream, "\n================================================================================\n");
        fprintf(stream, "%s\n", message);
        fprintf(stream, "================================================================================\n");
    }
}

void qlog_server_print_cmd_footer(FILE* stream){
    if (stream) {
        fprintf(stream, "================================================================================\n\n");
    }
}

