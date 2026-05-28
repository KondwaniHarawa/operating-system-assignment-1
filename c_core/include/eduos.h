#ifndef EDUOS_H
#define EDUOS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>

/* ── Process States ── */
#define NEW         0
#define READY       1
#define RUNNING     2
#define WAITING     3
#define TERMINATED  4

/* ── Thread Pool Size ── */
#define THREAD_POOL_SIZE 4

/* ── Process Control Block ── */
typedef struct {
    pid_t  pid;
    char   name[64];
    int    state;
    int    priority;
    int    burst_time;
    int    arrival_time;
    int    remaining_time;
    int    memory_req_kb;
    int    thread_count;
    time_t creation_time;
    int    exit_code;
    pid_t  parent_pid;
    int    owner_id;
} PCB;

/* ── Task for Thread Pool ── */
typedef struct Task {
    void (*function)(void *arg);
    void        *arg;
    struct Task *next;
} Task;

/* ── Thread Pool ── */
typedef struct {
    pthread_t       workers[THREAD_POOL_SIZE];
    Task           *head;
    Task           *tail;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
    int             shutdown;
    int             task_count;
} ThreadPool;

/* ── Function Declarations: process_manager.c ── */
pid_t edu_fork(PCB *parent);
void  edu_exec(pid_t pid, char *prog_name, int burst, int remaining);
int   edu_wait(pid_t parent_pid);
void  edu_exit(pid_t pid, int exit_code);
void  edu_ps(void);
void  write_pcb_snapshot(void);

/* ── Function Declarations: thread_manager.c ── */
ThreadPool *thread_pool_create(void);
void        thread_pool_submit(ThreadPool *pool, void (*func)(void *), void *arg);
void        thread_pool_destroy(ThreadPool *pool);
void        run_race_demo(void);
void        run_fixed_demo(void);
void        run_deadlock_demo(void);
void        run_producer_consumer(void);
void        run_many_to_one(void);
void        run_one_to_one(void);

/* ── Function Declarations: ipc_module.c ── */
void run_shared_memory_demo(void);
void run_pipe_demo(void);

#endif /* EDUOS_H */
