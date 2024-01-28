#ifndef PROD_CONS_TIMER_TIMER_H
#define PROD_CONS_TIMER_TIMER_H

#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include "queue.h"

typedef struct {
    int period;
    int tasksToExecute;
    int startDelay;
    void *(*startFcn)(void *arg);
    void *(*stopFcn)(void *arg);
    void *(*timerFcn)(void *arg);
    void *(*errorFcn)();
    void *userData;

    Queue *queue;
    pthread_t tid;
    void *(*producer)(void *arg);
    int *tJobIn;
    int *tDrift;
    pthread_mutex_t *tMut;
} Timer;

void timerInit(Timer *timer, int period, int tasksToExecute, void *(*stopFcn)(void *arg), void *(*timerFcn)(void *arg),
        void *(*errorFcn)(), Queue *queue, void *(*producer)(void *arg), int *tJobIn, int *tDrift,
        pthread_mutex_t *tMut);

void start(Timer *timer);

void startat(Timer *timer, int year, int month, int day, int hour, int minute, int second);

#endif //PROD_CONS_TIMER_TIMER_H