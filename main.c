#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>
#include "queue.h"
#include "timer.h"

void *producer(void *arg);
void *consumer(void *arg);
void *work(void *arg);
void *stop(void *arg);
void *error();
void saveT(int N, FILE *file, int *data);

typedef struct {
    Queue *queue;
    int *quit;
    int totalJobs;
    int *tJobWait;
    int *tJobOut;
    int *tJobDur;
    pthread_mutex_t *tMut;
} ConsumerArgs;

// Counts total lost jobs due to full queue.
int jobsLostCounter;

// Counts the number of jobs that the producers have added.
int jobsInCounter;

// Counts the number of jobs that the consumers have consumed.
int jobsOutCounter;

// Flag for the consumers to quit when needed.
volatile int quit;

int main () {

    // Initializes timers' period in milliseconds.
    const int period[3] = {1000, 100, 10};

    // Selects Queue Size.
    int queueSize = 0;
    printf("Select queue size: ");
    scanf("%d",&queueSize);
    if (queueSize<=0) {
        printf("Bad queue size!\n");
        return -1;
    }

    // Selects time (in seconds) to run the timers.
    int secondsToRun = 0;
    printf("Select seconds to run: ");
    scanf("%d",&secondsToRun);
    if (secondsToRun<=0) {
        printf("Bad seconds to run!\n");
        return -1;
    }

    // Selects mode.
    int mode = 0;
    printf("Available Options:\n");
    printf("1 - Timer with 1 sec period\n");
    printf("2 - Timer with 0.1 sec period\n");
    printf("3 - Timer with 0.01 sec period\n");
    printf("4 - All of the above\n");
    printf("Select Mode: ");
    scanf("%d", &mode);
    if (mode!=1 && mode!=2 && mode!=3 && mode!=4) {
        printf("Nonexistent Mode Selection!\n");
        return -1;
    }

    // Calculates total number of jobs.
    int totalJobs = 0;
    if (mode == 1)
        totalJobs = secondsToRun * (int)1e3 / period[0];
    else if (mode == 2)
        totalJobs = secondsToRun * (int)1e3 / period[1];
    else if (mode == 3)
        totalJobs = secondsToRun * (int)1e3 / period[2];
    else if (mode == 4)
        totalJobs = secondsToRun * (int)1e3 / period[0] + secondsToRun * (int)1e3 / period[1] + secondsToRun * (int)1e3 / period[2];

    //! Opens files for statistics.
    // Opens file to write time taken from the moment a job is pushed to the queue until it gets popped.
    FILE *fTJobWait = fopen("tJobWait.csv", "w");
    // Opens file to write time taken for a producer to push a job.
    FILE *fTJobIn = fopen("tJobIn.csv", "w");
    // Opens file to write time taken for a consumer to execute a job.
    FILE *fTJobDur = fopen("tJobDur.csv", "w");
    // Opens file to write each producer's time drifting.
    FILE *fTDrift, *fTDrift0, *fTDrift1, *fTDrift2;
    if (mode == 4) {
        fTDrift0 = fopen("tDrift0.csv", "w");
        fTDrift1 = fopen("tDrift1.csv", "w");
        fTDrift2 = fopen("tDrift2.csv", "w");
    }
    else
        fTDrift = fopen("tDrift.csv", "w");

    // Initializes random number seed.
    srand(time(NULL));

    for (int conNum=1; conNum<2; conNum*=2) {

        // Prints a message.
        printf("#Cons=%d Started.\n",conNum);

        //! Initializes variables.
        jobsLostCounter = 0;
        jobsInCounter = 0;
        jobsOutCounter = 0;

        // Flag begins as false.
        quit = 0;

        // Initializes Queue.
        Queue *fifo;
        fifo = queueInit(queueSize);
        if (fifo == NULL) {
            fprintf(stderr, "main: Queue Init failed.\n");
            exit(1);
        }

        //! Allocates memory for statistics with cells equal to the expected production.
        // tJobWait: Time taken from the moment a job is pushed to the queue until it gets popped.
        int *tJobWait = (int *) malloc (totalJobs * sizeof(int));
        // tJobIn: Time taken for a producer to push a job.
        int *tJobIn = (int *) malloc (totalJobs * sizeof(int));
        // tJobDur: Time taken for a consumer to execute a job.
        int *tJobDur = (int *) malloc (totalJobs * sizeof(int));
        // tDrift: Producer's time drifting.
        int *tDrift, *tDrift0, *tDrift1, *tDrift2;
        if (mode == 4) {
            tDrift0 = (int *) malloc (secondsToRun * (int)1e3 / period[0] * sizeof(int));
            tDrift1 = (int *) malloc (secondsToRun * (int)1e3 / period[1] * sizeof(int));
            tDrift2 = (int *) malloc (secondsToRun * (int)1e3 / period[2] * sizeof(int));
        }
        else
            tDrift = (int *) malloc (totalJobs * sizeof(int));

        // Initializes consumer threads arguments.
        ConsumerArgs *consArgs = (ConsumerArgs *) malloc (sizeof(ConsumerArgs));
        consArgs->queue = fifo;
        consArgs->totalJobs = totalJobs;
        consArgs->tJobWait = tJobWait;
        consArgs->tJobDur = tJobDur;
        consArgs->tMut = (pthread_mutex_t *) malloc (sizeof(pthread_mutex_t));
        pthread_mutex_init(consArgs->tMut, NULL);

        // Creates consumer threads.
        pthread_t con[conNum];
        for (int i=0; i<conNum; i++)
            pthread_create(&con[i], NULL, consumer, consArgs);

        // Creates timer(s).
        Timer *timer;
        pthread_mutex_t *tMut = (pthread_mutex_t *) malloc (sizeof(pthread_mutex_t));
        pthread_mutex_init(tMut, NULL);
        if (mode == 1) {
            timer = (Timer *) malloc (sizeof(Timer));
            timerInit(timer, period[0], secondsToRun * (int)1e3 / period[0], stop,
                    work, error, fifo, producer, tJobIn, tDrift, tMut);
            start(timer);
        }
        else if (mode == 2) {
            timer = (Timer *) malloc (sizeof(Timer));
            timerInit(timer, period[1], secondsToRun * (int)1e3 / period[1], stop,
                    work, error, fifo, producer, tJobIn, tDrift, tMut);
            start(timer);
        }
        else if (mode == 3) {
            timer = (Timer *) malloc (sizeof(Timer));
            timerInit(timer, period[2], secondsToRun * (int)1e3 / period[2], stop,
                    work, error, fifo, producer, tJobIn, tDrift, tMut);
            start(timer);
        }
        else if (mode == 4) {
            timer = (Timer *) malloc (3 * sizeof(Timer));
            timerInit(&timer[0], period[0], secondsToRun * (int)1e3 / period[0],
                    stop, work, error, fifo, producer, tJobIn, tDrift0, tMut);
            timerInit(&timer[1], period[1], secondsToRun * (int)1e3 / period[1],
                    stop, work, error, fifo, producer, tJobIn, tDrift1, tMut);
            timerInit(&timer[2], period[2], secondsToRun * (int)1e3 / period[2],
                    stop, work, error, fifo, producer, tJobIn, tDrift2, tMut);
            start(&timer[0]);
            start(&timer[1]);
            start(&timer[2]);
        }

        // Main waits for timer(s) to finish.
        if (mode == 4) {
            pthread_join(timer[0].tid, NULL);
            pthread_join(timer[1].tid, NULL);
            pthread_join(timer[2].tid, NULL);
        }
        else
            pthread_join(timer->tid, NULL);

        // Sets flag to 1 for the consumers to exit after they are "tricked" that queue is not empty.
        quit = 1;

        // Signals the consumer that queue is not empty, so they can quit safely.
        pthread_cond_broadcast(fifo->notEmpty);

        // Waits for threads to finish.
        for (int i=0; i<conNum; i++)
            pthread_join(con[i], NULL);

        // Prints a message.
        printf("#Cons=%d Ended. JobsLost=%d.\n", conNum, jobsLostCounter);

        //! Saves statistics. The number of row represents the number of consumers of the test.
        saveT(jobsOutCounter, fTJobWait, tJobWait);
        saveT(jobsOutCounter, fTJobIn, tJobIn);
        saveT(jobsOutCounter, fTJobDur, tJobDur);
        if (mode == 4) {
            saveT(secondsToRun * (int)1e3 / period[0] - 1, fTDrift0, tDrift0);
            saveT(secondsToRun * (int)1e3 / period[1] - 1, fTDrift1, tDrift1);
            saveT(secondsToRun * (int)1e3 / period[2] - 1, fTDrift2, tDrift2);
        }
        else
            saveT(jobsOutCounter-1, fTDrift, tDrift);

        //! Cleans up.
        free(tJobWait);
        free(tJobIn);
        free(tJobDur);
        if (mode == 4) {
            free(tDrift0);
            free(tDrift1);
            free(tDrift2);
        }
        else
            free(tDrift);
        free(timer);

        // Deletes Queue.
        queueDelete(fifo);

        pthread_mutex_destroy (consArgs->tMut);
        free(consArgs->tMut);
        free(consArgs);
        pthread_mutex_destroy (tMut);
        free(tMut);

        // Sleeps for 100ms before next iteration.
        usleep(1e5);

    }

    // Closes files.
    fclose(fTJobWait);
    fclose(fTJobIn);
    fclose(fTJobDur);
    if (mode == 4) {
        fclose(fTDrift0);
        fclose(fTDrift1);
        fclose(fTDrift2);
    }
    else
        fclose(fTDrift);

    return 0;
}

void *producer(void *arg) {
    Timer *timer = (Timer *)arg;

    // Initial Timer Delay.
    sleep(timer->startDelay);

    struct timeval tJobInStart, tJobInEnd, tProdExecStart, tProdExecEnd, tProdExecTemp;
    int tDriftTotal = 0;
    int driftCounter = -1;

    for (int i=0; i<timer->tasksToExecute; i++) {
        // Time drifting timestamps setup.
        gettimeofday(&tProdExecTemp, NULL);
        tProdExecStart = tProdExecEnd;
        tProdExecEnd = tProdExecTemp;

        // Creates the work function arguments. k is the number of them.
        gettimeofday(&tJobInStart, NULL);
        int k = (rand() % 101) + 100; // int k = 100;
        int *a = (int *) malloc ((k+1)*sizeof(int));
        a[0] = k;
        for (int j=0; j<k; j++)
            a[j+1] = k+j;

        // Creates the element that will be added to the queue.
        WorkFunction in;
        in.work = timer->timerFcn;
        in.arg = a;

        // Critical section begins.
        pthread_mutex_lock(timer->queue->mut);

        // Queue is full, so job is lost and errorFcn is executed.
        if (timer->queue->full) {
            //printf ("producer: queue FULL.\n");

            // Critical section ends.
            pthread_mutex_unlock(timer->queue->mut);

            // Signals the consumer that queue is not empty.
            pthread_cond_signal(timer->queue->notEmpty);

            // Critical section to run errorFcn starts.
            pthread_mutex_lock(timer->tMut);

            timer->errorFcn();

            // Critical section to run errorFcn ends.
            pthread_mutex_unlock(timer->tMut);
        }
        // Queue is not full, so the job is added to the queue.
        else {
            gettimeofday(&in.startTime, NULL);
            queueAdd(timer->queue, in);
            gettimeofday(&tJobInEnd, NULL);

            // Critical section ends.
            pthread_mutex_unlock(timer->queue->mut);

            // Signals the consumer that queue is not empty.
            pthread_cond_signal(timer->queue->notEmpty);

            // Critical section to write shared time statistics starts.
            pthread_mutex_lock(timer->tMut);

            // Calculates tJobIn.
            int tJobIn = (tJobInEnd.tv_sec-tJobInStart.tv_sec)*(int)1e6 + tJobInEnd.tv_usec-tJobInStart.tv_usec;
            timer->tJobIn[jobsInCounter++] = tJobIn;

            // Critical section to write shared time statistics ends.
            pthread_mutex_unlock(timer->tMut);
        }

        // Skip time drifting logic for first iteration.
        if (i==0) {
            usleep(timer->period*1e3);
            continue;
        }

        // Logic to face time drifting.
        int tDrift = (tProdExecEnd.tv_sec-tProdExecStart.tv_sec)*(int)1e6 + tProdExecEnd.tv_usec-tProdExecStart.tv_usec - timer->period*1e3;
        tDriftTotal += tDrift;
        //if (tDriftTotal<0)
        //    tDrift = 0;
        if (tDriftTotal>timer->period*(int)1e3)
            tDrift = timer->period*(int)1e3;
        else
            tDrift = tDriftTotal;

        timer->tDrift[++driftCounter] = tDriftTotal;
        usleep(timer->period*(int)1e3 - tDrift);
    }

    // Calls stop timer function.
    timer->stopFcn((void *) &timer->period);

    return NULL;;
}

void *consumer(void *arg) {
    ConsumerArgs *consArgs = (ConsumerArgs *)arg;

    struct timeval tJobWaitEnd, tJobDurStart, tJobDurEnd;
    WorkFunction out;
    double *r;

    while (1) {
        // Critical section begins.
        pthread_mutex_lock(consArgs->queue->mut);

        while (consArgs->queue->empty) {
            //printf ("consumer: Queue EMPTY.\n");
            pthread_cond_wait(consArgs->queue->notEmpty, consArgs->queue->mut);

            // Checks flag to quit when signaled.
            if (quit) {
                pthread_mutex_unlock(consArgs->queue->mut);
                return NULL;
            }
        }

        // Pops job from queue.
        queueDel(consArgs->queue, &out);
        gettimeofday(&tJobWaitEnd, NULL);

        // Critical section ends.
        pthread_mutex_unlock(consArgs->queue->mut);

        // Signals to producer that Queue is not full.
        pthread_cond_signal(consArgs->queue->notFull);

        // Executes work outside the critical section.
        gettimeofday(&tJobDurStart, NULL);
        r = (double *)out.work(out.arg);
        gettimeofday(&tJobDurEnd, NULL);

        // Frees work function arguments allocated dynamically from the producer and result allocated from the work function.
        free(out.arg);
        free(r);

        // Critical section to write shared time statistics starts.
        pthread_mutex_lock(consArgs->tMut);

        // Calculates tJobWait.
        int tJobWait = (tJobWaitEnd.tv_sec-out.startTime.tv_sec)*(int)1e6 + tJobWaitEnd.tv_usec-out.startTime.tv_usec;
        consArgs->tJobWait[jobsOutCounter] = tJobWait;
        // Calculates tJobDur.
        int tJobDur = (tJobDurEnd.tv_sec-tJobDurStart.tv_sec)*(int)1e6 + tJobDurEnd.tv_usec-tJobDurStart.tv_usec;
        consArgs->tJobDur[jobsOutCounter] = tJobDur;

        jobsOutCounter++;

        // Critical section to write shared time statistics ends.
        pthread_mutex_unlock(consArgs->tMut);
    }
}

void *work(void *arg) {
    int *a = (int *)arg;
    double *r = (double *) malloc (sizeof(double));
    *r = 0;
    for (int i=0; i<a[0]; i++)
        *r += sin((double)a[i+1]);

    return r;
}

void *stop(void *arg) {
    int *period = (int *) arg;
    printf("Timer with period %d stopped.\n", *period);
}

void *error() {
    jobsLostCounter++;
}

void saveT(int N, FILE *file, int *data) {
    for (int i=0; i<N; i++)
        fprintf(file, "%d,", data[i]);
    fprintf(file, "\n");
}
