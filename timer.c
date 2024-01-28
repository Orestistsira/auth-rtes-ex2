#include "timer.h"

void timerInit(Timer *timer, int period, int tasksToExecute, void *(*stopFcn)(void *arg), void *(*timerFcn)(void *arg),
        void *(*errorFcn)(), Queue *queue, void *(*producer)(void *arg), int *tJobIn, int *tDrift,
        pthread_mutex_t *tMut) {
    timer->period = period;
    timer->tasksToExecute = tasksToExecute;
    timer->startDelay = 0;
    timer->startFcn = NULL;
    timer->stopFcn = stopFcn;
    timer->timerFcn = timerFcn;
    timer->errorFcn = errorFcn;
    timer->userData = NULL;

    timer->queue = queue;
    timer->producer = producer;
    timer->tJobIn = tJobIn;
    timer->tDrift = tDrift;
    timer->tMut = tMut;
}

void start(Timer *timer) {
    pthread_create(&timer->tid, NULL, timer->producer, timer);
}

void startat(Timer *timer, int year, int month, int day, int hour, int minute, int second) {
    int delay = 0;

    time_t now = time(NULL);

    struct tm startAt;
    startAt.tm_year = year;
    startAt.tm_mon = month;
    startAt.tm_mday = day;
    startAt.tm_hour = hour;
    startAt.tm_min = minute;
    startAt.tm_sec = second;

    // Calculates difference of the desired time to start the timer and the current time.
    delay = (int) difftime(now, mktime(&startAt));

    if (delay<0)
        delay = 0;
    timer->startDelay = delay;
    pthread_create(&timer->tid, NULL, timer->producer, timer);
}