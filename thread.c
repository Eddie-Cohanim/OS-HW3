#include "thread.h"
#include <stdlib.h>

Thread_stats createThreadStats(int threadsNum) {
    Thread_stats newThreadStats = (Thread_stats)malloc(sizeof(Thread_stats));

    newThreadStats->m_totalReq = 0;
    newThreadStats->m_dynamicReq = 0;
    newThreadStats->m_staticReq = 0;
    newThreadStats->m_id = threadsNum;

    return newThreadStats;
}

Thread* createThread(int threadsNum){
    Thread* newThread = (Thread*)malloc(sizeof(Thread));
    newThread->m_threadInUse = threadsNum;
    return newThread;
}

int getNumThreads(Thread *thread){
    return thread->m_threadInUse;
}

void increase(Thread *thread){
    thread->m_threadInUse++;
}

void decrease(Thread *thread){
    thread->m_threadInUse--;
}