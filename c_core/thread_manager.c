#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define THREAD_POOL_SIZE 4

pthread_t threads[THREAD_POOL_SIZE];
pthread_mutex_t lock;

int counter = 0;

void *race_task(void *arg)
{
    for (int i = 0; i < 100000; i++)
    {
        counter++;
    }
    return NULL;
}

void *fixed_task(void *arg)
{
    for (int i = 0; i < 100000; i++)
    {
        pthread_mutex_lock(&lock);
        counter++;
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void run_race_demo()
{
    counter = 0;

    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_create(&threads[i], NULL, race_task, NULL);
    }

    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("%d\n", counter);
}

void run_fixed_demo()
{
    counter = 0;
    pthread_mutex_init(&lock, NULL);

    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_create(&threads[i], NULL, fixed_task, NULL);
    }

    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);

    printf("%d\n", counter);
}

int main()
{
    run_race_demo();
    run_fixed_demo();
    return 0;
}
