#ifndef TIME_CMP
#define TIME_CMP

#include <RTClib.h>

void TIME_init();
DateTime TIME_now();
bool TIME_tick(unsigned long duration);
void TIME_update();
void TIME_printClock();

extern unsigned long prevMillis;

#endif