#include "thread.h"
#include <stdlib.h>

Thread_stats* createThreadStats(int threadsNum) {
    Thread_stats* newThreadStats = (Thread_stats*)malloc(sizeof(Thread_stats));

    newThreadStats->m_totalReq = 0;
    newThreadStats->m_dynamicReq = 0;
    newThreadStats->m_staticReq = 0;
    newThreadStats->m_id = threadsNum;

    return newThreadStats;
}