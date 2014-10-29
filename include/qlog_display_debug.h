#ifndef __QLOG_DISPLAY_DEBUG_H
#define __QLOG_DISPLAY_DEBUG_H

void qlog_display_debug_print_buffer_id(FILE* stream, qlog_buffer_id_t buffer_id);
void qlog_display_debug_print_all_buffers(FILE* stream, int print_status, int print_events);

#endif
