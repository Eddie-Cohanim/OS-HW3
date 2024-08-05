#ifndef TASK_3_OS_QUEUE_H
#define TASK_3_OS_QUEUE_H
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <assert.h>


typedef struct Queue{
    int m_size;
    struct Node* m_headNode;
    struct Node* m_endNode;
} Queue;


typedef struct Node{
    int m_connFd;
    struct Node* m_nextNode;
    struct Node* m_prevNode;
    struct timeval m_arrival;
} Node;

Node* createNode(int connFd, struct timeval arrival);
Node* getNodeInIndex(Queue* queue, int index);
void removeNodeFromQueue(Queue* queue, Node* node);
void removeNodeFromQueueWithoutDeletingIt(Queue* queue, Node* node);


Queue* createQueue();
Node* addToQueue(Queue* queue, int connFd, struct timeval arrival);
Node* popQueue(Queue* queue);
Node* popLastInQueue(Queue* queue);
int getSize(Queue* queue);




#endif //TASK_3_OS_QUEUE_H
