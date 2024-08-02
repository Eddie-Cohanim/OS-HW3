#include "segel.h"
#include "request.h"
#include "queue.h"
#include <pthread.h>
#include "thread.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// HW3: Parse the new arguments too

enum SCHEDULER_ALGORITHM {
    BLOCK,
    DROP_TAIL,
    DROP_HEAD,
    BLOCK_FLUSH,
    RANDOM
};

pthread_mutex_t queueLock;
pthread_cond_t QueueNoWait;
pthread_cond_t queueOverload;
pthread_cond_t queueBlockFlush;


void getargs(int *port, int argc, char *argv[], int* totalNumOfThreads, int* queueSize, enum SCHEDULER_ALGORITHM* schedulerAlgorithem)
{
    if (argc < 5) {
	    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	    exit(1);
    }

    if(strcmp(argv[4], "random") == 0){
        *schedulerAlgorithem = RANDOM;
    }
    else if(strcmp(argv[4], "bf") == 0){
        *schedulerAlgorithem = BLOCK_FLUSH;
    }
    else if(strcmp(argv[4], "dh") == 0){
        *schedulerAlgorithem = DROP_HEAD;
    }   
    else if(strcmp(argv[4], "dt") == 0){
        *schedulerAlgorithem = DROP_TAIL;
    }
    else if(strcmp(argv[4], "block") == 0){
        *schedulerAlgorithem = BLOCK;
    }
    else{
        //something went wrong
    }


    *queueSize = atoi(argv[3]);
    *totalNumOfThreads = atoi(argv[2]);
    *port = atoi(argv[1]);
}



void* workerThread(void* data, Queue* pendingRequestsQueue, Queue* ongoingRequestsQueue){
    
    Thread_stats* stats = (Thread_stats*)data;
    struct timeval dispatchTime, currentTime;
    while(1){
        
        pthread_mutex_lock(&queueLock);
        while(getSize(pendingRequestsQueue) == 0){
            pthread_cond_wait(&QueueNoWait, &queueLock);
        }
        Node* request = popQueue(pendingRequestsQueue);
        addToQueue(ongoingRequestsQueue, request->m_connFd, request->m_arrival);
        pthread_mutex_unlock(&queueLock);
        gettimeofday(&currentTime, NULL);
        timersub(&currentTime, &request->m_arrival, &dispatchTime);
        requestHandle(request->m_connFd, request->m_arrival, dispatchTime, stats);
        close(request->m_connFd);
        
        pthread_mutex_lock(&queueLock);
        removeNodeFromQueue(ongoingRequestsQueue, request);
        
        pthread_cond_signal(&queueOverload);
        if(getSize(pendingRequestsQueue) + getSize(ongoingRequestsQueue) == 0){
            pthread_cond_signal(&queueBlockFlush);
        }
        pthread_mutex_unlock(&queueLock);

        free(request);

    }
}



/*******************Scheduler Algorithm Implementations******************/


void blockAlgorithm(Queue* pendingRequestsQueue, Queue* ongoingRequestsQueue, int queueSize, int connfd,struct timeval timeOfArival){
  while (getSize(pendingRequestsQueue) + getSize(ongoingRequestsQueue) >= queueSize) {
        pthread_cond_wait(&queueOverload, &queueLock);
    }
    addToQueue(pendingRequestsQueue, connfd, timeOfArival);
    pthread_cond_signal(&QueueNoWait);
    return;
}

void dropTailALgorithem(int connfd){
    close(connfd);
}

void dropHeadAlgorithem(Queue* pendingRequestsQueue, int queueSize, int connfd,struct timeval timeOfArival){
    if(getSize(pendingRequestsQueue) == 0){
         Close(connfd);
         return;
    }
    Node *head = popQueue(pendingRequestsQueue);
    close(head->m_connFd);
    free(head);
    addToQueue(pendingRequestsQueue, connfd, timeOfArival);
    pthread_cond_signal(&QueueNoWait);

    return;
}

void blockFlushAlgorithm(Queue* pendingRequestsQueue, Queue* ongoingRequestsQueue,int connfd){
    while (getSize(pendingRequestsQueue) + getSize(ongoingRequestsQueue) != 0) {
        pthread_cond_wait(&queueBlockFlush, &queueLock);
    }
    close(connfd);
    pthread_cond_signal(&QueueNoWait);

}

void randomAlgorithem(Queue* pendingRequestsQueue, int connfd, int queueSize, struct timeval timeOfArrival){
    if(queueSize == 0){
        close(connfd);
        return;
    }

    int numOfRequestsToDrop = (queueSize + 1) / 2;
    for (int i = numOfRequestsToDrop; i > 0; i--){
        int requestIndexToDrop = rand() % queueSize;
        Node* nodeToRemove = getNodeInIndex(pendingRequestsQueue, requestIndexToDrop);
        close(nodeToRemove->m_connFd);
        removeNodeFromQueue(pendingRequestsQueue, nodeToRemove);
        free (nodeToRemove);
    }

    addToQueue(pendingRequestsQueue, connfd, timeOfArrival);
}


/********************************Main************************************/


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, totalNumOfThreads, queueSize;
    struct sockaddr_in clientaddr;
    enum SCHEDULER_ALGORITHM schedulerAlgorithem;
    
    getargs(&port, argc, argv, &totalNumOfThreads, &queueSize, &schedulerAlgorithem);
    pthread_t* threads = (pthread_t*)malloc(totalNumOfThreads*sizeof(pthread_t));
    Thread_stats** threadStats = (Thread_stats**)malloc(totalNumOfThreads*sizeof(Thread_stats*));
    Queue *pendingRequestsQueue = createQueue();
    Queue *ongoingRequestsQueue = createQueue();
    pthread_mutex_init(&queueLock, NULL);
    pthread_cond_init(&queueOverload, NULL);
    pthread_cond_init(&QueueNoWait, NULL);
    pthread_cond_init(&queueBlockFlush, NULL);

    for(int i = 0; i <totalNumOfThreads; i++){
        threadStats[i] = (Thread_stats*)malloc(sizeof(Thread_stats));
        threadStats[i]->m_id = i;
        threadStats[i]->m_dynamicReq = 0;
        threadStats[i]->m_staticReq = 0;
        threadStats[i]->m_totalReq = 0;
        pthread_create(&threads[i], NULL, workerThread, threadStats[i]);
    }

    // 
    // HW3: Create some threads...
    //
    struct timeval timeOfArival;
    
    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        gettimeofday(&timeOfArival , NULL);

        pthread_mutex_lock(&queueLock);

        if(getSize(pendingRequestsQueue) + getSize(ongoingRequestsQueue) < queueSize){
            addToQueue(pendingRequestsQueue, connfd, timeOfArival);
        }else{
            if(schedulerAlgorithem == BLOCK){
                blockAlgorithm(pendingRequestsQueue, ongoingRequestsQueue, queueSize, connfd, timeOfArival);
            }
            else if(schedulerAlgorithem == DROP_HEAD){
                dropHeadAlgorithem(pendingRequestsQueue,queueSize,connfd,timeOfArival);
            }
            else if(schedulerAlgorithem == DROP_TAIL){
                dropTailALgorithem(connfd);
            }
            else if(schedulerAlgorithem == BLOCK_FLUSH){
                blockFlushAlgorithm(pendingRequestsQueue, ongoingRequestsQueue,connfd);
            }
            else if(schedulerAlgorithem == RANDOM){
                randomAlgorithem(pendingRequestsQueue, connfd, queueSize, timeOfArival);
            }
        }
        pthread_mutex_unlock(&queueLock);

        // 
        // HW3: In general, don't handle the request in the main thread.
        // Save the relevant info in a buffer and have one of the worker threads 
        // do the work. 
        // 

        //Close(connfd);
    }

}


    


 
