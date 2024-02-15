#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <pthread.h>

#define THREAD_ADD_STEP 2

typedef struct Task
{
    void (*func)(void* arg);
    void* arg;
}Task;

typedef struct ThreadPool
{
    Task* taskQ;
    int queueCapacity;
    int queueSize;   
    int queueFront;  // first element of queue -> pop data
    int queueRear;   // last element of queue -> push data

    pthread_t managerID;  // the manager thread
    pthread_t* threadIDs; // the ptr to thread arrays
    int minThreadNum;
    int maxThreadNum;
    int busyThreadNum;
    int liveThreadNum;
    int toExitThreadNum;

    pthread_mutex_t mutexPool;
    pthread_mutex_t mutexBusy; // lock to busyThreadNum
    pthread_cond_t notFull;
    pthread_cond_t notEmpty;

    int shutDown;
}ThreadPool;

ThreadPool* createThreadPool(int minThreads, int maxThreads, int queueSize);
int destroyThreadPool(ThreadPool *pool);
int getBusyThreadNum(ThreadPool* pool);
int getLiveThreadNum(ThreadPool* pool);
void addTask(ThreadPool* pool, void (*taskFunc)(void*), void* taskArg);

#endif