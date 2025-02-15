#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

extern void log_data(FILE* stream, unsigned char* buf, size_t len);

extern unsigned long long get_current_time_ms();

#endif