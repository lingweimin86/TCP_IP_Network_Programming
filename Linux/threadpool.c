#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "threadpool.h"

void* workerFunc(void* arg);
//

void* managerFunc(void* arg)
{
    ThreadPool* pool = (ThreadPool* )arg;

    while(!pool->shutDown)
    {
        // check every 3 seconds
        sleep(3);

        pthread_mutex_lock(&pool->mutexPool);
        int queueSize = pool->queueSize;
        int liveNum = pool->liveThreadNum;
        pthread_mutex_unlock(&pool->mutexPool);

        // add thread when task count is greater than live thread num
        if (queueSize > liveNum && liveNum < pool->maxThreadNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            int count = 0;
            for (int i = 0; i < pool->maxThreadNum && 
                count < THREAD_ADD_STEP && 
                pool->liveThreadNum < pool->maxThreadNum; i++)
            {
                if (pool->threadIDs[i] == 0)
                {      
                    pthread_create(&pool->threadIDs[i], NULL, workerFunc, pool);
                    count++;
                    pool->liveThreadNum++;
                    printf("too less threads, create 1 more.\n");
                    printf("current live threads: %d.\n", pool->liveThreadNum);

                }
            }
            pthread_mutex_unlock(&pool->mutexPool);         
        }

        pthread_mutex_lock(&pool->mutexBusy);
        int busyNum = pool->busyThreadNum;
        pthread_mutex_unlock(&pool->mutexBusy);

        // destroy thread when busy thread num * 2 < live thread num
        if (busyNum * 2 < liveNum && liveNum > pool->minThreadNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            pool->toExitThreadNum = THREAD_ADD_STEP;
            pthread_mutex_unlock(&pool->mutexPool); 

            for (int i = 0; i < THREAD_ADD_STEP; i++)
            {
                pthread_cond_signal(&pool->notEmpty); // send one signal a time
                printf("too many threads, destroy 1 thread.\n");
            }
            
        }

    }
}

void workerExit(ThreadPool* pool)
{
    pthread_mutex_lock(&pool->mutexPool);

    for (int i = 0; i < pool->maxThreadNum; i++)
    {
        if (pool->threadIDs[i] == pthread_self())
        {
            pool->liveThreadNum--;
            printf("thread %ld finished.\n", pool->threadIDs[i]);
            printf("current live threads: %d.\n", pool->liveThreadNum);
            pool->threadIDs[i] = 0;
            break;
        }
    }

    pthread_mutex_unlock(&pool->mutexPool);

    pthread_exit(NULL);
}

void* workerFunc(void* arg)
{
    ThreadPool* pool = (ThreadPool* )arg;

    printf("worker thread created. thread id: %ld\n", pthread_self());

    while (1)
    {
        pthread_mutex_lock(&pool->mutexPool);
        while (pool->queueSize == 0 && !pool->shutDown) // ensure only wake up one thread at a time
        {
            printf("the task queue is empty. worker thread is blocking...\n");
            pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);

            if (pool->toExitThreadNum > 0)
            {
                pool->toExitThreadNum--;
                pthread_mutex_unlock(&pool->mutexPool);
                workerExit(pool);
            }

        }

        if (pool->shutDown)
        {
            pthread_mutex_unlock(&pool->mutexPool);
            workerExit(pool);
        }

        Task task;

        task.func = pool->taskQ[pool->queueFront].func;
        task.arg = pool->taskQ[pool->queueFront].arg;
        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        pool->queueSize--;
        pthread_cond_signal(&pool->notFull);
        pthread_mutex_unlock(&pool->mutexPool);

        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyThreadNum++;
        pthread_mutex_unlock(&pool->mutexBusy);

        task.func(task.arg);
        free(task.arg);
        task.arg = NULL;

        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyThreadNum--;
        pthread_mutex_unlock(&pool->mutexBusy);
    }  
}

void taskFunc(void* arg)
{
    int taskId = *(int* )arg;
    int sleepInSeconds = (taskId+1)%11;
    sleep(sleepInSeconds);
    printf("task completed. taskId: %d, cost time: %ds\n", taskId, sleepInSeconds);
}

// create pool
ThreadPool* createThreadPool(int minThreads, int maxThreads, int queueSize)
{
    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    do {
        if (pool == NULL)
        {
            printf("malloc threadpool fail...\n");
            break;
        }

        if (minThreads > maxThreads)
        {
            printf("minTreads can not be greater than maxThreads...\n");
            break;
        }

        pool->threadIDs = (pthread_t* )malloc(sizeof(pthread_t) * maxThreads);
        if (pool->threadIDs == NULL)
        {
            printf("malloc threadIDs fail...\n");
            break;
        }

        memset(pool->threadIDs, 0, sizeof(pthread_t)*maxThreads);
        pool->minThreadNum = minThreads;
        pool->maxThreadNum = maxThreads;
        pool->busyThreadNum = 0;
        pool->liveThreadNum = minThreads;
        pool->toExitThreadNum = 0;

        if (pthread_mutex_init(&pool->mutexPool, NULL) != 0 || 
            pthread_mutex_init(&pool->mutexBusy, NULL) != 0 || 
            pthread_cond_init(&pool->notFull, NULL) != 0 || 
            pthread_cond_init(&pool->notEmpty, NULL) != 0)
        {
            printf("init mutex/condition fail...\n");
            break;
        }

        // task Q
        pool->taskQ = (Task* )malloc(sizeof(Task) * queueSize);
        if (pool->taskQ == NULL)
        {
            printf("malloc taskQ fail...\n");
            break;
        }

        pool->queueCapacity = queueSize;
        pool->queueSize = 0;
        pool->queueFront = 0;
        pool->queueRear = 0;
        pool->shutDown = 0;

        // create threads
        pthread_create(&pool->managerID, NULL, managerFunc, pool);
        for (int i = 0; i < minThreads; i++)
        {
            pthread_create(&pool->threadIDs[i], NULL, workerFunc, pool);
        }
        

        printf("thread pool created.\n");
        return pool;
    } while (0);

    // free resource when fail
    if (pool && pool->taskQ) { free(pool->taskQ); }
    if (pool && pool->threadIDs) { free(pool->threadIDs); }
    if (pool) { free(pool); }

    return NULL;
}

// destroy pool

int destroyThreadPool(ThreadPool *pool)
{
    if (pool == NULL)
    {
        return -1;
    }

    pool->shutDown = 1;

    pthread_join(pool->managerID, NULL);

    for (int i = 0; i < pool->liveThreadNum; i++)
    {
        pthread_cond_signal(&pool->notEmpty);
    }
    
    if (pool->taskQ) { free(pool->taskQ); }
    if (pool->threadIDs) { free(pool->threadIDs); }

    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_mutex_destroy(&pool->mutexPool);
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);

    free(pool);
    pool = NULL;

    printf("good bye\n");
    return 0;
}

// add task to pool
void addTask(ThreadPool* pool, void (*taskFunc)(void*), void* taskArg)
{
    pthread_mutex_lock(&pool->mutexPool);

    while (pool->queueSize == pool->queueCapacity && !pool->shutDown)
    {
        printf("the task queue is full. adding task is blocking...\n");
        pthread_cond_wait(&pool->notFull, &pool->mutexPool);    
    }

    if (pool->shutDown)
    {
        pthread_mutex_unlock(&pool->mutexPool);
        return;
    }

    pool->taskQ[pool->queueRear].func = taskFunc;
    pool->taskQ[pool->queueRear].arg = taskArg;
    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
    pool->queueSize++;

    pthread_cond_signal(&pool->notEmpty);
    pthread_mutex_unlock(&pool->mutexPool);
}

// get busy thread num
int getBusyThreadNum(ThreadPool* pool)
{
    pthread_mutex_lock(&pool->mutexBusy);
    int busyNum = pool->busyThreadNum;
    pthread_mutex_unlock(&pool->mutexBusy);

    return busyNum;
}

// get live thread num
int getLiveThreadNum(ThreadPool* pool)
{
    pthread_mutex_lock(&pool->mutexPool);
    int liveNum = pool->liveThreadNum;
    pthread_mutex_unlock(&pool->mutexPool);

    return liveNum;
}

int main(int argc, char const *argv[])
{
    ThreadPool* pool = createThreadPool(2, 10, 8);

    for (int i = 0; i < 50; i++)
    {
        //sleep(1);
        int* arg = malloc(sizeof(int));
        *arg = i;
        addTask(pool, taskFunc, (void *)arg);
    }
    
    sleep(30);

    for (int i = 50; i < 100; i++)
    {
        //sleep(2);
        int* arg = malloc(sizeof(int));
        *arg = i;
        addTask(pool, taskFunc, (void *)arg);
    }
    
    destroyThreadPool(pool);
    return 0;
}
