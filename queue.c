#include "queue.h"

/*********************Node Function Implementations**********************/
Node* createNode(int connFd, struct timeval arrival){
    Node* newNode = (Node*)malloc(sizeof(Node));
    if(newNode == NULL)
    {
        //do something
    }
    newNode->m_nextNode = NULL;
    newNode->m_prevNode = NULL;
    newNode->m_arrival = arrival;
    newNode->m_connFd = connFd;
    return newNode;
}

Node* getNodeInIndex(Queue* queue, int index) {
    if(queue == NULL)
    {
        //do something
    }
    if(index < 1 || index > queue->m_size)
    {
        //do something
    }
    Node* currentNode = queue->m_headNode;
    for (int i = 0; i < index; i++) {
        currentNode = currentNode->m_nextNode;
    }
    return currentNode;
}

void removeNodeFromQueue(Queue* queue, Node* node) {
    if(queue == NULL)
    {
        //do something
    }
    if(node == NULL)
    {
        //do something
    }
    Node* currentNode = queue->m_headNode;
    while (currentNode != NULL) {
        if (currentNode == node) {
            if (currentNode->m_nextNode != NULL) {
                currentNode->m_nextNode->m_prevNode = currentNode->m_prevNode;
            }
            if (currentNode->m_prevNode != NULL) {
                currentNode->m_prevNode->m_nextNode = currentNode->m_nextNode;
            }
            if (currentNode == queue->m_headNode) {
                queue->m_headNode = currentNode->m_nextNode;
            }
            if (currentNode == queue->m_endNode) {
                queue->m_endNode = currentNode->m_prevNode;
            }
            queue->m_size--;
            free(currentNode);
            return;
        }
        currentNode = currentNode->m_nextNode;
    }
    //node wasnt found probably need to do somthing about it
}

void removeNodeFromQueueWithoutDeletingIt(Queue* queue, Node* node){
    if(queue == NULL)
    {
        //do something
    }
    if(node == NULL)
    {
        //do something
    }
    Node* currentNode = queue->m_headNode;
    while (currentNode != NULL) {
        if (currentNode == node) {
            if (currentNode->m_nextNode != NULL) {
                currentNode->m_nextNode->m_prevNode = currentNode->m_prevNode;
            }
            if (currentNode->m_prevNode != NULL) {
                currentNode->m_prevNode->m_nextNode = currentNode->m_nextNode;
            }
            if (currentNode == queue->m_headNode) {
                queue->m_headNode = currentNode->m_nextNode;
            }
            if (currentNode == queue->m_endNode) {
                queue->m_endNode = currentNode->m_prevNode;
            }
            queue->m_size--;
            return;
        }
        currentNode = currentNode->m_nextNode;
    }
    //node wasnt found probably need to do somthing about it
}


/*********************Queue Function Implementations*********************/


Queue* createQueue(){
    Queue* newQueue = (Queue*)malloc(sizeof(Queue));
    if(newQueue == NULL)
    {
        //do something
    }
    newQueue->m_headNode = NULL;
    newQueue->m_endNode = NULL;
    newQueue->m_size = 0;
    return newQueue;
}

Node* addToQueue(Queue* queue, int connFd, struct timeval arrival){
    if(queue == NULL)
    {
        //do something
    }

    Node* newNode = createNode(connFd, arrival);

    if (queue->m_size == 0) {
        queue->m_headNode = newNode;
        queue->m_endNode = newNode;
    } 
    else {
        newNode->m_prevNode = queue->m_endNode;
        queue->m_endNode->m_nextNode = newNode;
        queue->m_endNode = newNode;
    }

    queue->m_size++;
    return newNode;
}

Node* popQueue(Queue* queue){

    if(queue == NULL){
        //do something
    }
    if(getSize(queue) <= 0){
        //do something
    }

    Node* headNode = queue->m_headNode;

    if (getSize(queue) > 1) {
        queue->m_headNode = headNode->m_nextNode;
        queue->m_headNode->m_prevNode = NULL;
        queue->m_size--;
    }
    else {
        queue->m_endNode = NULL;
        queue->m_headNode = NULL;
        queue->m_size--;

    }
    
    return headNode;
}

Node* popLastInQueue(Queue* queue){
    if(queue == NULL){
        //do something
    }
    if(getSize(queue) <= 0){
        //do something
    }

    Node* endNode = queue->m_endNode;

    removeNodeFromQueueWithoutDeletingIt(queue, endNode);

    return endNode;
}

int getSize(Queue *queue){
    if(queue == NULL)
    {
        //do something
    }
    return queue->m_size;
}

