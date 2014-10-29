/*
 * Copyright (c) 2014 Jozsef Galajda <jgalajda@pannongsm.hu>
 * All rights reserved.
 */

#ifndef __QLOG_DISPLAY_H
#define __QLOG_DISPLAY_H

void qlog_display_event(FILE* stream, const qlog_event_t* event);
void qlog_display_format_timestamp(char* buffer, size_t size, const struct timeval* t);
void qlog_display_format_event_str(const qlog_event_t* event, char* buffer, size_t buffer_size);
void qlog_display_print_buffer_id(FILE* stream, qlog_buffer_id_t buffer_id);
void qlog_display_print_buffer(FILE* stream);
void qlog_display_print_buffer_list(FILE* stream);


#endif
