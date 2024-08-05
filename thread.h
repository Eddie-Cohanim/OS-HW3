#ifndef WET3_BACKUP_THREAD_H
#define WET3_BACKUP_THREAD_H

typedef struct Thread_stats{
    int m_totalReq;
    int m_dynamicReq;
    int m_staticReq;
    int m_id;

} *Thread_stats;

Thread_stats createThreadStats(int threadsNum);

#endif //WET3_BACKUP_THREAD_H
