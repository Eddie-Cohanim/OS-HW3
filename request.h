#ifndef __REQUEST_H__
#include "thread.h"

void requestHandle(int fd, struct timeval arrival, struct timeval dispatch, Thread_stats t_stats);

#endif
