#include <stdio.h>
#include <string.h>
#include <execinfo.h>
#include <stdlib.h>


void qlog_ext_display_hex_dump(FILE* stream, void* datap, size_t size){
    unsigned int i = 0, j = 0;
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


void qlog_ext_display_bt(FILE* stream, void* data, size_t size){
    size_t i;
    char **strings;
    size_t num_of_stacks = size / sizeof(void*);
    void** bt = (void**)data;

    strings = backtrace_symbols(bt, num_of_stacks);

    for (i = 0; i < num_of_stacks; i++){
        fprintf(stream, "\tframe#%-3lu: %s\n", i, strings[i]);
    }

    free (strings);
}

