//http://stackoverflow.com/questions/5635362/max-thread-per-process-in-linux

//compile with g++ threadtest.cpp -pthread -m32 -march=i686 on stoker

#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

// this prevents the compiler from reordering code over this COMPILER_BARRIER
// this doesnt do anything
#define COMPILER_BARRIER() __asm__ __volatile__ ("" ::: "memory")

sigset_t    _fSigSet;
volatile int    _cActive = 0;
pthread_t   thrd[1000000];

void * thread(void *i)
{
int nSig, cActive;

    cActive = __sync_fetch_and_add(&_cActive, 1);
    COMPILER_BARRIER();  // make sure the active count is incremented before sigwait

    // sigwait is a handy way to sleep a thread and wake it on command
    sigwait(&_fSigSet, &nSig); //make the thread still alive

    COMPILER_BARRIER();  // make sure the active count is decrimented after sigwait
    cActive = __sync_fetch_and_add(&_cActive, -1);
    //printf("%d(%d) ", i, cActive);
    return 0;
}

int main(int argc, char** argv)
{
pthread_attr_t attr;
int cThreadRequest, cThreads, i, err, cActive, cbStack;

    cbStack = (argc > 1) ? atoi(argv[1]) : 0x100000;
    cThreadRequest = (argc > 2) ? atoi(argv[2]) : 30000;

    sigemptyset(&_fSigSet);
    sigaddset(&_fSigSet, SIGUSR1);
    sigaddset(&_fSigSet, SIGSEGV);

    printf("Start\n");
    pthread_attr_init(&attr);
    if ((err = pthread_attr_setstacksize(&attr, cbStack)) != 0)
        printf("pthread_attr_setstacksize failed: err: %d %s\n", err, strerror(err));

    for (i = 0; i < cThreadRequest; i++)
    {
        if ((err = pthread_create(&thrd[i], &attr, thread, (void*)i)) != 0)
        {
            printf("pthread_create failed on thread %d, error code: %d %s\n", 
                     i, err, strerror(err));
            break;
        }
    }
    cThreads = i;

    printf("\n");
  // wait for threads to all be created, although we might not wait for 
    // all threads to make it through sigwait
    while (1)
    {
        cActive = _cActive;
        if (cActive == cThreads)
            break;
        printf("Waiting A %d/%d,", cActive, cThreads);
        sched_yield();
    }

    // wake em all up so they exit
    for (i = 0; i < cThreads; i++)
        pthread_kill(thrd[i], SIGUSR1);

    // wait for them all to exit, although we might be able to exit before 
    // the last thread returns
    while (1)
    {
        cActive = _cActive;
        if (!cActive)
            break;
        printf("Waiting B %d/%d,", cActive, cThreads);
        sched_yield();
    }

    printf("\nDone. Threads requested: %d.  Threads created: %d.  StackSize=%lfmb\n", 
     cThreadRequest, cThreads, (double)cbStack/0x100000);
    return 0;
}
