#include "segel.h"
#include "request.h"
#include "queue.h"
#include <pthread.h>// might not need cause its in request.h now
#include "thread.h"

#define ARGS_SIZE 5

pthread_mutex_t queueLock;
pthread_cond_t isPendingQueueEmpty;
pthread_cond_t isBufferAvailable;
pthread_cond_t areBothEmpty;



Queue* pendingRequestsQueue;
static int threadsInUse = 0;
Thread_stats* threadsStats;


enum SCHEDULER_ALGORITHM {
    BLOCK,
    DROP_TAIL,
    DROP_HEAD,
    BLOCK_FLUSH,
    RANDOM
};


void initializeMutexAndCond() {
    pthread_mutex_init(&queueLock, NULL);
    pthread_cond_init(&isPendingQueueEmpty, NULL);
    pthread_cond_init(&areBothEmpty, NULL);
    pthread_cond_init(&isBufferAvailable, NULL);
}

void getargs(int *port,int *threads_num,int* queue_size,enum SCHEDULER_ALGORITHM *schedalg, int argc, char *argv[])
{
    if (argc < ARGS_SIZE) {
        fprintf(stderr, "Usage: %s <portnum> <threads> <queue_size> <schedalg>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *threads_num = atoi(argv[2]);
    *queue_size = atoi(argv[3]);

    if (strcmp(argv[4], "block") == 0) {
        *schedalg = BLOCK;
    }
     else if (strcmp(argv[4], "dt") == 0) {
        *schedalg = DROP_TAIL;
    }
     else if (strcmp(argv[4], "dh") == 0) {
        *schedalg = DROP_HEAD;
    }
     else if (strcmp(argv[4], "bf") == 0) {
        *schedalg = BLOCK_FLUSH;
    }
     else if (strcmp(argv[4], "random") == 0) {
        *schedalg = RANDOM;
    }
     else {
        exit(1);
    }
}

void *processRequest(void *arg){
    int threadIndex = *((int *) arg);

    while(1){
        pthread_mutex_lock(&queueLock);
        while(getSize(pendingRequestsQueue) == 0){
            pthread_cond_wait(&isPendingQueueEmpty, &queueLock);
        }
        Node* requestNode = popQueue(pendingRequestsQueue);

        struct timeval handle;
        gettimeofday(&handle, NULL);
        struct timeval dispatch_time;
        timersub(&handle, &requestNode->m_arrival, &dispatch_time);
        threadsInUse++;
        pthread_mutex_unlock(&queueLock);
        requestHandle(requestNode->m_connFd, requestNode->m_arrival, dispatch_time, threadsStats[threadIndex],
                      pendingRequestsQueue, requestNode, queueLock, isBufferAvailable);
        threadsInUse--;
        pthread_mutex_lock(&queueLock);
        pthread_cond_signal(&isBufferAvailable);
        if (getSize(pendingRequestsQueue) == 0 && threadsInUse == 0){
            pthread_cond_signal(&areBothEmpty); // for block_flush sched algorithm
        }
        pthread_mutex_unlock(&queueLock);
    }
}


void blockAlgorithm(int queueSize){
    while (getSize(pendingRequestsQueue) + threadsInUse == queueSize) {
        pthread_cond_wait(&isBufferAvailable, &queueLock);
}
}

void dropTailAlgorithm(int connFd){
    Close(connFd);
    pthread_mutex_unlock(&queueLock);
}


int dropHeadAlgorithm(int connFd){
    if (getSize(pendingRequestsQueue) != 0) {
        Node* head = popQueue(pendingRequestsQueue);
        Close(head->m_connFd);
        return 0;
    }
    else{
        Close(connFd);
        pthread_mutex_unlock(&queueLock);
        return 1;
    }
}

void blockFlashAlgorithm(int connFd){
    while (getSize(pendingRequestsQueue) + threadsInUse > 0) {
        pthread_cond_wait(&areBothEmpty, &queueLock);
    }
    Close(connFd);
    pthread_mutex_unlock(&queueLock);
}

int randomAlgorithm(int connFd){
    if (getSize(pendingRequestsQueue) == 0) {
        Close(connFd);
        pthread_mutex_unlock(&queueLock);
        return 0;
        }

    int numToDrop = (int) ((getSize(pendingRequestsQueue) + 1) / 2);

    for (int i = 0; i < numToDrop; i++) {
        int element_index_to_drop = rand() % getSize(pendingRequestsQueue);
        Node* node_to_drop = getNodeInIndex(pendingRequestsQueue, element_index_to_drop);
        Close(node_to_drop->m_connFd);
        removeNodeFromQueue(pendingRequestsQueue, node_to_drop);
        
    }
    return 1;
}



int main(int argc, char *argv[])
{
    int listenFd, connFd, port, clientLenAdd,threadsNum, queueSize;
    enum SCHEDULER_ALGORITHM schedAlg;
    struct sockaddr_in clientAddr;
    initializeMutexAndCond();

    getargs(&port, &threadsNum, &queueSize, &schedAlg, argc, argv);
    pendingRequestsQueue = createQueue();

    pthread_t *threads = malloc(threadsNum * sizeof(pthread_t));
    threadsStats = malloc(threadsNum * sizeof(Thread_stats));

    for (int i = 0; i < threadsNum; i++) {
        int* threadIndex = malloc(sizeof(int));
        *threadIndex = i;
        pthread_create(&threads[i], NULL, processRequest, (void*)threadIndex);
        threadsStats[i] = createThreadStats(i);
    }

    listenFd = Open_listenfd(port);

    while (1) {
        clientLenAdd = sizeof(clientAddr);
        connFd = Accept(listenFd, (SA *)&clientAddr, (socklen_t *) &clientLenAdd);
        struct timeval arrival;
        gettimeofday(&arrival, NULL);
        pthread_mutex_lock(&queueLock);

        if (getSize(pendingRequestsQueue) + threadsInUse == queueSize) {
            if (schedAlg == BLOCK) {
                blockAlgorithm(queueSize);
            } else if (schedAlg == DROP_TAIL) {
                dropTailAlgorithm(connFd);
                continue;
            } else if (schedAlg == DROP_HEAD) {
                if(dropHeadAlgorithm(connFd) ==1){
                    continue;
                }
            } else if (schedAlg == BLOCK_FLUSH) {
                blockFlashAlgorithm(connFd);
                continue;
            } else if (schedAlg == RANDOM) {
                if(randomAlgorithm(connFd) ==0){
                    continue;
                }   
            }
        }
        addToQueue(pendingRequestsQueue, connFd, arrival);
        pthread_cond_signal(&isPendingQueueEmpty);
        pthread_mutex_unlock(&queueLock);
    }
}