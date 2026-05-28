#include "include/eduos.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <semaphore.h>

/* ═══════════════════════════════════════════════
   SHARED MEMORY IPC DEMO
   Two processes write/read a shared struct
   containing process metrics via shm_open/mmap.
   Access control: only matching owner_id allowed.
   ═══════════════════════════════════════════════ */

#define SHM_NAME "/eduos_shm"

typedef struct {
    pthread_mutex_t lock;
    int             owner_id;
    int             pid;
    char            name[64];
    int             burst_time;
    int             memory_req_kb;
    int             state;
} SharedMetrics;

void run_shared_memory_demo(void) {
    printf("\n[SHM] Shared Memory IPC demonstration:\n");

    /* Create shared memory region */
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) { perror("shm_open"); return; }

    if (ftruncate(fd, sizeof(SharedMetrics)) == -1) {
        perror("ftruncate"); close(fd); return;
    }

    SharedMetrics *shm = mmap(NULL, sizeof(SharedMetrics),
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) { perror("mmap"); close(fd); return; }
    close(fd);

    /* Initialise mutex for shared memory coordination */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shm->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    /* Set owner_id — only this owner may read/write */
    shm->owner_id = 42;

    pid_t child = fork();
    if (child == -1) { perror("fork"); return; }

    if (child == 0) {
        /* ── CHILD: Writer ── */
        int requester_id = 42; /* must match owner_id */

        pthread_mutex_lock(&shm->lock);

        /* Access control check */
        if (shm->owner_id != requester_id) {
            printf("[SHM] CHILD: Access DENIED (owner=%d, requester=%d)\n",
                   shm->owner_id, requester_id);
            pthread_mutex_unlock(&shm->lock);
            munmap(shm, sizeof(SharedMetrics));
            exit(1);
        }

        shm->pid          = getpid();
        shm->burst_time   = 25;
        shm->memory_req_kb = 512;
        shm->state        = 2; /* RUNNING */
        strncpy(shm->name, "ChildProcess", sizeof(shm->name) - 1);

        printf("[SHM] CHILD (PID=%d): wrote metrics to shared memory\n",
               getpid());

        pthread_mutex_unlock(&shm->lock);
        munmap(shm, sizeof(SharedMetrics));
        exit(0);

    } else {
        /* ── PARENT: Reader ── */
        waitpid(child, NULL, 0);

        pthread_mutex_lock(&shm->lock);

        int requester_id = 42;
        if (shm->owner_id != requester_id) {
            printf("[SHM] PARENT: Access DENIED\n");
            pthread_mutex_unlock(&shm->lock);
        } else {
            printf("[SHM] PARENT: Read from shared memory:\n");
            printf("      PID=%d  Name=%s  Burst=%d  Memory=%dKB  State=%d\n",
                   shm->pid, shm->name, shm->burst_time,
                   shm->memory_req_kb, shm->state);
            pthread_mutex_unlock(&shm->lock);
        }

        pthread_mutex_destroy(&shm->lock);
        munmap(shm, sizeof(SharedMetrics));
        shm_unlink(SHM_NAME);
    }
}

/* ═══════════════════════════════════════════════
   ANONYMOUS PIPE IPC DEMO
   Parent serialises PCB data and sends to child
   via pipe(). Child parses and prints it.
   ═══════════════════════════════════════════════ */

void run_pipe_demo(void) {
    printf("\n[PIPE] Anonymous Pipe IPC demonstration:\n");

    /* Build a sample PCB to serialise */
    PCB sample;
    sample.pid           = 99;
    sample.state         = 2; /* RUNNING */
    sample.priority      = 1;
    sample.burst_time    = 30;
    sample.arrival_time  = 0;
    sample.remaining_time = 20;
    sample.memory_req_kb = 256;
    sample.thread_count  = 2;
    sample.creation_time = time(NULL);
    sample.exit_code     = 0;
    sample.parent_pid    = 1;
    sample.owner_id      = 42;
    strncpy(sample.name, "PipeTestProcess", sizeof(sample.name) - 1);

    /* Serialise PCB to a string */
    char buf[512];
    snprintf(buf, sizeof(buf),
             "pid=%d|name=%s|state=%d|priority=%d|"
             "burst=%d|arrival=%d|remaining=%d|"
             "memory=%d|threads=%d|exit=%d",
             sample.pid, sample.name, sample.state,
             sample.priority, sample.burst_time,
             sample.arrival_time, sample.remaining_time,
             sample.memory_req_kb, sample.thread_count,
             sample.exit_code);

    int pipefd[2];
    if (pipe(pipefd) == -1) { perror("pipe"); return; }

    pid_t child = fork();
    if (child == -1) { perror("fork"); return; }

    if (child == 0) {
        /* ── CHILD: Reader ── */
        close(pipefd[1]); /* close write end */

        char rbuf[512];
        ssize_t n = read(pipefd[0], rbuf, sizeof(rbuf) - 1);
        if (n == -1) { perror("read"); exit(1); }
        rbuf[n] = '\0';
        close(pipefd[0]);

        printf("[PIPE] CHILD received serialised PCB:\n      %s\n", rbuf);

        /* Parse key=value pairs */
        printf("[PIPE] CHILD parsed fields:\n");
        char tmp[512];
        strncpy(tmp, rbuf, sizeof(tmp) - 1);
        char *token = strtok(tmp, "|");
        while (token) {
            printf("      %s\n", token);
            token = strtok(NULL, "|");
        }
        exit(0);

    } else {
        /* ── PARENT: Writer ── */
        close(pipefd[0]); /* close read end */

        ssize_t w = write(pipefd[1], buf, strlen(buf));
        if (w == -1) perror("write");

        close(pipefd[1]);

        printf("[PIPE] PARENT sent %zd bytes through pipe\n", w);
        waitpid(child, NULL, 0);
    }
}
