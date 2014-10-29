#ifndef __QLOG_DEBUG_H
#define __QLOG_DEBUG_H

qlog_buffer_t* qlog_dbg_get_default_buffer(void);
void qlog_dbg_print_event(FILE* stream, qlog_event_t* event);
void qlog_dbg_print_buffer(FILE* stream, qlog_buffer_t* buffer);
void qlog_dbg_print_buffers(FILE* stream);

#endif
