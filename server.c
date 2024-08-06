#include "segel.h"
#include "request.h"
#include "queue.h"
#include <pthread.h>
#include "thread.h"

#define ARGS_SIZE 5

pthread_mutex_t queueLock;
pthread_cond_t isPendingQueueEmpty;
pthread_cond_t isBufferAvailable;
pthread_cond_t areBothEmpty;



Queue* pendingRequestsQueue;
Thread* thread;
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

void getargs(int *port,int *threadsNum,int* queueSize,enum SCHEDULER_ALGORITHM *schedalgorithm, int argc, char *argv[])
{
    if (argc < ARGS_SIZE) {
        fprintf(stderr, "Usage: %s <portnum> <threads> <queue_size> <schedalg>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *threadsNum = atoi(argv[2]);
    *queueSize = atoi(argv[3]);

    if (strcmp(argv[4], "block") == 0) {
        *schedalgorithm = BLOCK;
    }
     else if (strcmp(argv[4], "dt") == 0) {
        *schedalgorithm = DROP_TAIL;
    }
     else if (strcmp(argv[4], "dh") == 0) {
        *schedalgorithm = DROP_HEAD;
    }
     else if (strcmp(argv[4], "bf") == 0) {
        *schedalgorithm = BLOCK_FLUSH;
    }
     else if (strcmp(argv[4], "random") == 0) {
        *schedalgorithm = RANDOM;
    }
     else {
        exit(1);
    }
}

struct timeval timeHandler(Node* requestNode){
    struct timeval handle, dispatchTime;
    gettimeofday(&handle, NULL);
    timersub(&handle, &requestNode->m_arrival, &dispatchTime);
    return dispatchTime;
}

void *processRequest(void *arg){
    int threadIndex = *((int *) arg);
    while(1){
        pthread_mutex_lock(&queueLock);
        while(getSize(pendingRequestsQueue) == 0){
            pthread_cond_wait(&isPendingQueueEmpty, &queueLock);
        }
        Node* requestNode = popQueue(pendingRequestsQueue);
        pthread_mutex_unlock(&queueLock);
        increase(thread);
        requestHandle(requestNode->m_connFd, requestNode->m_arrival, timeHandler(requestNode), threadsStats[threadIndex], false,
                      pendingRequestsQueue, requestNode, queueLock, isBufferAvailable);
        Close(requestNode->m_connFd);
        free(requestNode);
        pthread_mutex_lock(&queueLock);
        pthread_cond_signal(&isBufferAvailable);
        decrease(thread);

        if (getSize(pendingRequestsQueue) == 0 && getNumThreads(thread) == 0){
            pthread_cond_signal(&areBothEmpty); // for block_flush sched algorithm
        }
        pthread_mutex_unlock(&queueLock);
    }
}

void blockAlgorithm(int queueSize){
    while (getSize(pendingRequestsQueue) + getNumThreads(thread) == queueSize) {
        pthread_cond_wait(&isBufferAvailable, &queueLock);
}
}

void dropTailAlgorithm(int connFd){
    Close(connFd);
    pthread_mutex_unlock(&queueLock);
}

int dropHeadAlgorithm(int connFd){
    if(getSize(pendingRequestsQueue) == 0){
        Close(connFd);
        pthread_mutex_unlock(&queueLock);
        return 1;
    }
    else {
        Node* head = popQueue(pendingRequestsQueue);
        Close(head->m_connFd);
        return 0;
    }
}

void blockFlashAlgorithm(int connFd){
    while (getSize(pendingRequestsQueue) + getNumThreads(thread) > 0) {
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

    for (int i = numToDrop; i > 0; i--) {
        int elementToDrop = rand() % getSize(pendingRequestsQueue);
        Node* nodeToDrop = getNodeInIndex(pendingRequestsQueue, elementToDrop);
        Close(nodeToDrop->m_connFd);
        removeNodeFromQueue(pendingRequestsQueue, nodeToDrop);
        
    }
    return 1;
}

int main(int argc, char *argv[])
{
    int listenFd, connFd, port, clientLenAdd,threadsNum, queueSize;
    enum SCHEDULER_ALGORITHM schedalgorithm;
    struct sockaddr_in clientAddr;
    initializeMutexAndCond();

    getargs(&port, &threadsNum, &queueSize, &schedalgorithm, argc, argv);
    pendingRequestsQueue = createQueue();
    thread = createThread(0);

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

        if (getSize(pendingRequestsQueue) + getNumThreads(thread) == queueSize) {
            if (schedalgorithm == BLOCK) {
                blockAlgorithm(queueSize);
            } 
            else if (schedalgorithm == DROP_TAIL) {
                dropTailAlgorithm(connFd);
                continue;
            } 
            else if (schedalgorithm == DROP_HEAD) {
                if(dropHeadAlgorithm(connFd) == 1){
                    continue;
                }
            } 
            else if (schedalgorithm == BLOCK_FLUSH) {
                blockFlashAlgorithm(connFd);
                continue;
            } 
            else if (schedalgorithm == RANDOM) {
                if(randomAlgorithm(connFd) == 0){
                    continue;
                }   
            }
        }
        addToQueue(pendingRequestsQueue, connFd, arrival);
        pthread_cond_signal(&isPendingQueueEmpty);
        pthread_mutex_unlock(&queueLock);
    }

    if (close(connFd) < 0) {//do we need it? chatgpt thinks so
        //perror("Error closing listen socket");
        exit(1);
    }

    if (close(listenFd) < 0) {//do we need it? chatgpt thinks so
        //perror("Error closing listen socket");
        exit(1);
    }
}