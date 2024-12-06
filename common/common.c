#include "common.h"

void log_data(FILE* stream, unsigned char* buf, unsigned int len){
    unsigned int i;
    for(i = 0; i < len; ++i){
        fprintf(stream, "0x%02X ", buf[i]);
        if((i+1)%16 == 0){
            fprintf(stream, "\n");
        }
    }
    if((i+1)%16){
        fprintf(stream, "\n");
    }
}