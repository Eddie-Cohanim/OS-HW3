#ifndef __REQUEST_H__
#include "thread.h"
#include "queue.h"
#include <pthread.h>


void requestHandle(int fd, struct timeval arrival, struct timeval dispatch, Thread_stats t_stats,
                   Queue* pendingRequestsQueueu, Node* requestNode, pthread_mutex_t queueLock, pthread_cond_t isBufferAvailable);

#endif