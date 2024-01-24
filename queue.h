#ifndef PROD_CONS_TIMER_QUEUE_H
#define PROD_CONS_TIMER_QUEUE_H

#include <stdlib.h>
#include <pthread.h>

typedef struct {
    void *(*work)(void *);
    void *arg;
    struct timeval startTime;
} WorkFunction;

typedef struct {
    WorkFunction *buf;
    long size, head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
} Queue;

Queue *queueInit(int size);

void queueDelete(Queue *q);

void queueAdd(Queue *q, WorkFunction in);

void queueDel(Queue *q, WorkFunction *out);

#endif //PROD_CONS_TIMER_QUEUE_H
