#ifndef HELPER_TIME_H
#define HELPER_TIME_H

#include <ctime>

uint64_t getMonoticMS() ;
int32_t nextTimerMS() ;
void processTimers() ;

#endif