#ifndef WET3_BACKUP_THREAD_H
#define WET3_BACKUP_THREAD_H

typedef struct Thread_stats{
    int m_totalReq;
    int m_dynamicReq;
    int m_staticReq;
    int m_id;

} *Thread_stats;

Thread_stats createThreadStats(int threadsNum);

typedef struct Thread{
   int m_threadInUse; 
}Thread;

Thread* createThread(int threadsNum);

int getNumThreads(Thread* thread);
void increase(Thread* thread);
void decrease(Thread* thread);



#endif //WET3_BACKUP_THREAD_H