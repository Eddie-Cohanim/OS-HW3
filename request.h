#ifndef __REQUEST_H__
#include "thread.h"
#include "queue.h"

void requestHandle(int fd, struct timeval arrival, struct timeval dispatch,
                   Thread_stats t_stats, Queue* pendingRequestsQueueu);

#endif