#include "common.h"
#include "time.h"

void log_data(FILE* stream, unsigned char* buf, size_t len){
    size_t i = 0;
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

unsigned long long get_current_time_ms() {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (unsigned long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}