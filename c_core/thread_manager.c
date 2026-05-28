#include "include/eduos.h"
#include <semaphore.h>

/* ═══════════════════════════════════════════════
   THREAD POOL (One-to-One model — each EduOS
   thread maps to one POSIX pthread)
   ═══════════════════════════════════════════════ */

static void *worker_thread(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;

    while (1) {
        pthread_mutex_lock(&pool->lock);

        while (pool->task_count == 0 && !pool->shutdown)
            pthread_cond_wait(&pool->cond, &pool->lock);

        if (pool->shutdown && pool->task_count == 0) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        Task *task  = pool->head;
        pool->head  = task->next;
        if (pool->head == NULL)
            pool->tail = NULL;
        pool->task_count--;

        pthread_mutex_unlock(&pool->lock);

        task->function(task->arg);
        free(task);
    }
    return NULL;
}

ThreadPool *thread_pool_create(void) {
    ThreadPool *pool = malloc(sizeof(ThreadPool));
    if (!pool) { perror("malloc ThreadPool"); exit(EXIT_FAILURE); }

    pool->head       = NULL;
    pool->tail       = NULL;
    pool->shutdown   = 0;
    pool->task_count = 0;

    if (pthread_mutex_init(&pool->lock, NULL) != 0) {
        perror("pthread_mutex_init"); exit(EXIT_FAILURE);
    }
    if (pthread_cond_init(&pool->cond, NULL) != 0) {
        perror("pthread_cond_init"); exit(EXIT_FAILURE);
    }

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        if (pthread_create(&pool->workers[i], NULL, worker_thread, pool) != 0) {
            perror("pthread_create"); exit(EXIT_FAILURE);
        }
    }

    printf("[ThreadPool] Created with %d worker threads (One-to-One model)\n",
           THREAD_POOL_SIZE);
    return pool;
}

void thread_pool_submit(ThreadPool *pool, void (*func)(void *), void *arg) {
    Task *task = malloc(sizeof(Task));
    if (!task) { perror("malloc Task"); exit(EXIT_FAILURE); }

    task->function = func;
    task->arg      = arg;
    task->next     = NULL;

    pthread_mutex_lock(&pool->lock);

    if (pool->tail)
        pool->tail->next = task;
    else
        pool->head = task;
    pool->tail = task;
    pool->task_count++;

    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->lock);
}

void thread_pool_destroy(ThreadPool *pool) {
    pthread_mutex_lock(&pool->lock);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->lock);

    for (int i = 0; i < THREAD_POOL_SIZE; i++)
        pthread_join(pool->workers[i], NULL);

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond);
    free(pool);
    printf("[ThreadPool] Gracefully shut down\n");
}

/* ═══════════════════════════════════════════════
   RACE CONDITION DEMONSTRATION
   make race  → no mutex (non-deterministic)
   make fixed → with mutex (always correct)
   ═══════════════════════════════════════════════ */

#define NUM_THREADS  4
#define INCREMENTS   100000

static pthread_t      demo_threads[NUM_THREADS];
static pthread_mutex_t counter_lock;
static int shared_counter = 0;

static void *race_task(void *arg) {
    (void)arg;
    for (int i = 0; i < INCREMENTS; i++)
        shared_counter++;
    return NULL;
}

static void *fixed_task(void *arg) {
    (void)arg;
    for (int i = 0; i < INCREMENTS; i++) {
        pthread_mutex_lock(&counter_lock);
        shared_counter++;
        pthread_mutex_unlock(&counter_lock);
    }
    return NULL;
}

void run_race_demo(void) {
    shared_counter = 0;
    printf("\n[RACE] Running WITHOUT mutex (expected=%d):\n",
           NUM_THREADS * INCREMENTS);

    for (int i = 0; i < NUM_THREADS; i++)
        if (pthread_create(&demo_threads[i], NULL, race_task, NULL) != 0)
            perror("pthread_create");

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(demo_threads[i], NULL);

    printf("[RACE] Result = %d (non-deterministic!)\n", shared_counter);
}

void run_fixed_demo(void) {
    shared_counter = 0;
    pthread_mutex_init(&counter_lock, NULL);
    printf("\n[FIXED] Running WITH mutex (expected=%d):\n",
           NUM_THREADS * INCREMENTS);

    for (int i = 0; i < NUM_THREADS; i++)
        if (pthread_create(&demo_threads[i], NULL, fixed_task, NULL) != 0)
            perror("pthread_create");

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(demo_threads[i], NULL);

    printf("[FIXED] Result = %d (always correct)\n", shared_counter);
    pthread_mutex_destroy(&counter_lock);
}

/* ═══════════════════════════════════════════════
   DEADLOCK DEMONSTRATION & FIX
   Always acquire mutex_A before mutex_B
   ═══════════════════════════════════════════════ */

static pthread_mutex_t mutex_A;
static pthread_mutex_t mutex_B;

static void *fixed_thread1(void *arg) {
    (void)arg;
    pthread_mutex_lock(&mutex_A);
    pthread_mutex_lock(&mutex_B);
    printf("[FIXED DEADLOCK] Thread1 acquired A then B safely\n");
    pthread_mutex_unlock(&mutex_B);
    pthread_mutex_unlock(&mutex_A);
    return NULL;
}

static void *fixed_thread2(void *arg) {
    (void)arg;
    pthread_mutex_lock(&mutex_A);
    pthread_mutex_lock(&mutex_B);
    printf("[FIXED DEADLOCK] Thread2 acquired A then B safely\n");
    pthread_mutex_unlock(&mutex_B);
    pthread_mutex_unlock(&mutex_A);
    return NULL;
}

void run_deadlock_demo(void) {
    pthread_mutex_init(&mutex_A, NULL);
    pthread_mutex_init(&mutex_B, NULL);

    printf("\n[DEADLOCK] Fixed demo — consistent lock ordering (A → B):\n");
    pthread_t t1, t2;
    pthread_create(&t1, NULL, fixed_thread1, NULL);
    pthread_create(&t2, NULL, fixed_thread2, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    pthread_mutex_destroy(&mutex_A);
    pthread_mutex_destroy(&mutex_B);
}

/* ═══════════════════════════════════════════════
   PRODUCER-CONSUMER using semaphores
   ═══════════════════════════════════════════════ */

#define BUFFER_SIZE 5

static int    buffer[BUFFER_SIZE];
static int    buf_in  = 0;
static int    buf_out = 0;
static sem_t  sem_empty;
static sem_t  sem_full;
static pthread_mutex_t buf_mutex;

static void *producer(void *arg) {
    (void)arg;
    for (int i = 1; i <= 10; i++) {
        sem_wait(&sem_empty);
        pthread_mutex_lock(&buf_mutex);
        buffer[buf_in] = i;
        buf_in = (buf_in + 1) % BUFFER_SIZE;
        printf("[PRODUCER] Produced: %d\n", i);
        pthread_mutex_unlock(&buf_mutex);
        sem_post(&sem_full);
    }
    return NULL;
}

static void *consumer(void *arg) {
    (void)arg;
    for (int i = 1; i <= 10; i++) {
        sem_wait(&sem_full);
        pthread_mutex_lock(&buf_mutex);
        int val = buffer[buf_out];
        buf_out = (buf_out + 1) % BUFFER_SIZE;
        printf("[CONSUMER] Consumed: %d\n", val);
        pthread_mutex_unlock(&buf_mutex);
        sem_post(&sem_empty);
    }
    return NULL;
}

void run_producer_consumer(void) {
    printf("\n[SEM] Producer-Consumer with semaphores:\n");
    sem_init(&sem_empty, 0, BUFFER_SIZE);
    sem_init(&sem_full,  0, 0);
    pthread_mutex_init(&buf_mutex, NULL);

    pthread_t prod, cons;
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);
    pthread_mutex_destroy(&buf_mutex);
}

/* ═══════════════════════════════════════════════
   MANY-TO-ONE MODEL SIMULATION
   Cooperative scheduler — all threads share
   one kernel thread. A blocking call blocks all.
   ═══════════════════════════════════════════════ */

#define M2O_THREADS 3

void run_many_to_one(void) {
    printf("\n[M2O] Many-to-One threading model demonstration:\n");
    printf("[M2O] All user threads share ONE kernel thread.\n");
    printf("[M2O] A blocking call in any thread blocks ALL.\n\n");

    int done[M2O_THREADS] = {0};
    int step[M2O_THREADS] = {0};
    int all_done = 0;

    while (!all_done) {
        all_done = 1;
        for (int i = 0; i < M2O_THREADS; i++) {
            if (!done[i]) {
                all_done = 0;
                printf("[M2O] User-thread %d running (step %d) — single kernel thread\n",
                       i, step[i] + 1);
                step[i]++;
                if (step[i] >= 3)
                    done[i] = 1;
            }
        }
    }
    printf("[M2O] All user-threads completed cooperatively.\n");
}

/* ═══════════════════════════════════════════════
   ONE-TO-ONE MODEL: parallel summation
   Each EduOS thread = one POSIX pthread
   ═══════════════════════════════════════════════ */

#define SUM_SIZE 1000000

static long long partial_sums[THREAD_POOL_SIZE];

typedef struct { int start; int end; int id; } SumArg;

static void *parallel_sum(void *arg) {
    SumArg *s = (SumArg *)arg;
    long long sum = 0;
    for (int i = s->start; i <= s->end; i++)
        sum += i;
    partial_sums[s->id] = sum;
    return NULL;
}

void run_one_to_one(void) {
    printf("\n[O2O] One-to-One model: parallel summation of 1..%d\n", SUM_SIZE);

    pthread_t threads[THREAD_POOL_SIZE];
    SumArg    args[THREAD_POOL_SIZE];
    int chunk = SUM_SIZE / THREAD_POOL_SIZE;

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        args[i].start = i * chunk + 1;
        args[i].end   = (i == THREAD_POOL_SIZE - 1) ? SUM_SIZE : (i + 1) * chunk;
        args[i].id    = i;
        pthread_create(&threads[i], NULL, parallel_sum, &args[i]);
    }

    long long total = 0;
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(threads[i], NULL);
        total += partial_sums[i];
    }

    long long expected = (long long)SUM_SIZE * (SUM_SIZE + 1) / 2;
    printf("[O2O] Total = %lld (expected %lld) — %s\n",
           total, expected, total == expected ? "CORRECT" : "WRONG");
}
