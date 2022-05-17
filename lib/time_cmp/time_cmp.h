#ifndef TIME_CMP
#define TIME_CMP

#include <RTClib.h>

void TIME_init();
DateTime TIME_now();

extern unsigned long prevMillis;

#endif