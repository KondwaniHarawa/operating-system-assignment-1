#include "include/eduos.h"
#include <sys/wait.h>

/* ── External declarations ── */
extern PCB pcb_table[100];
extern int pcb_count;

/* ── Thread pool task example ── */
static void sample_task(void *arg) {
    int id = *(int *)arg;
    printf("[TASK] Worker task %d executing\n", id);
    sleep(1);
    printf("[TASK] Worker task %d done\n", id);
    free(arg);
}

int main(void) {
    printf("╔══════════════════════════════════════╗\n");
    printf("║         EduOS Simulator v1.0         ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    /* ════════════════════════════════════════
       PART 1: Process Management
       ════════════════════════════════════════ */
    printf("━━━ PART 1: Process Management ━━━\n\n");

    /* Create init process */
    PCB init;
    init.pid            = 0;
    init.parent_pid     = 0;
    init.state          = READY;
    init.priority       = 0;
    init.burst_time     = 10;
    init.arrival_time   = 0;
    init.remaining_time = 10;
    init.memory_req_kb  = 128;
    init.thread_count   = 1;
    init.creation_time  = time(NULL);
    init.exit_code      = 0;
    init.owner_id       = 42;
    strncpy(init.name, "init", sizeof(init.name) - 1);

    /* Fork two child processes */
    pid_t child1 = edu_fork(&init);
    pid_t child2 = edu_fork(&init);

    /* exec on child1 */
    edu_exec(child1, "scheduler_daemon", 20, 20);

    /* exec on child2 */
    edu_exec(child2, "memory_manager",   15, 15);

    /* Fork a grandchild from child1 */
    PCB *c1 = NULL;
    for (int i = 0; i < pcb_count; i++)
        if (pcb_table[i].pid == child1) { c1 = &pcb_table[i]; break; }
    if (c1) {
        pid_t child3 = edu_fork(c1);
        edu_exec(child3, "worker_thread", 5, 5);
        edu_exit(child3, 0);
    }

    /* Print process table */
    printf("\n── edu_ps output ──\n");
    edu_ps();

    /* Terminate children */
    edu_exit(child1, 0);
    edu_exit(child2, 0);

    /* Wait for all children */
    edu_wait(init.pid);

    printf("\n── edu_ps after termination ──\n");
    edu_ps();

    /* ════════════════════════════════════════
       PART 2: Thread Pool (One-to-One model)
       ════════════════════════════════════════ */
    printf("━━━ PART 2: Thread Pool ━━━\n\n");

    ThreadPool *pool = thread_pool_create();

    for (int i = 0; i < 6; i++) {
        int *id = malloc(sizeof(int));
        if (!id) { perror("malloc"); exit(EXIT_FAILURE); }
        *id = i + 1;
        thread_pool_submit(pool, sample_task, id);
    }

    /* Allow tasks to complete before shutdown */
    sleep(3);
    thread_pool_destroy(pool);

    /* ════════════════════════════════════════
       PART 3: Threading Models
       ════════════════════════════════════════ */
    printf("\n━━━ PART 3: Threading Models ━━━\n");

    /* Many-to-One */
    run_many_to_one();

    /* One-to-One parallel summation */
    run_one_to_one();

    /* ════════════════════════════════════════
       PART 4: Race Condition Demo
       ════════════════════════════════════════ */
    printf("\n━━━ PART 4: Synchronisation ━━━\n");

    run_race_demo();
    run_fixed_demo();
    run_deadlock_demo();
    run_producer_consumer();

    /* ════════════════════════════════════════
       PART 5: IPC Demo
       ════════════════════════════════════════ */
    printf("\n━━━ PART 5: IPC Demo ━━━\n");

    run_shared_memory_demo();
    run_pipe_demo();

    /* ════════════════════════════════════════
       Final PCB snapshot for Python scheduler
       ════════════════════════════════════════ */
    write_pcb_snapshot();
    printf("\n[INFO] pcb_snapshot.json written — ready for Python scheduler\n");
    printf("\n╔══════════════════════════════════════╗\n");
    printf("║       EduOS Simulation Complete      ║\n");
    printf("╚══════════════════════════════════════╝\n");

    return 0;
}
