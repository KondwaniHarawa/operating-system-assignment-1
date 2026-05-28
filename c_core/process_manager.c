#include "include/eduos.h"
#include <time.h>

/* ── Global PCB Table ── */
PCB pcb_table[100];
int pcb_count  = 0;
int next_pid   = 1;

/* ── Helper: state name ── */
static const char *state_name(int s) {
    switch (s) {
        case NEW:        return "NEW";
        case READY:      return "READY";
        case RUNNING:    return "RUNNING";
        case WAITING:    return "WAITING";
        case TERMINATED: return "TERMINATED";
        default:         return "UNKNOWN";
    }
}

/* ── Helper: timestamp string ── */
static void timestamp(char *buf, size_t len) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", t);
}

/* ── Helper: find PCB by PID ── */
static PCB *find_pcb(pid_t pid) {
    for (int i = 0; i < pcb_count; i++)
        if (pcb_table[i].pid == pid)
            return &pcb_table[i];
    return NULL;
}

/* ── JSON Serialiser ── */
void write_pcb_snapshot(void) {
    FILE *f = fopen("pcb_snapshot.json", "w");
    if (!f) { perror("fopen pcb_snapshot.json"); return; }

    fprintf(f, "[\n");
    for (int i = 0; i < pcb_count; i++) {
        PCB *p = &pcb_table[i];
        fprintf(f,
            "  {\n"
            "    \"pid\": %d,\n"
            "    \"name\": \"%s\",\n"
            "    \"state\": \"%s\",\n"
            "    \"priority\": %d,\n"
            "    \"burst_time\": %d,\n"
            "    \"arrival_time\": %d,\n"
            "    \"remaining_time\": %d,\n"
            "    \"memory_req_kb\": %d,\n"
            "    \"thread_count\": %d,\n"
            "    \"exit_code\": %d,\n"
            "    \"parent_pid\": %d,\n"
            "    \"owner_id\": %d,\n"
            "    \"creation_time\": %ld\n"
            "  }%s\n",
            p->pid, p->name, state_name(p->state),
            p->priority, p->burst_time, p->arrival_time,
            p->remaining_time, p->memory_req_kb, p->thread_count,
            p->exit_code, p->parent_pid, p->owner_id,
            (long)p->creation_time,
            (i < pcb_count - 1) ? "," : ""
        );
    }
    fprintf(f, "]\n");
    fclose(f);
}

/* ── edu_fork ── */
pid_t edu_fork(PCB *parent) {
    if (pcb_count >= 100) {
        fprintf(stderr, "[ERROR] PCB table full\n");
        return -1;
    }

    char ts[32];
    timestamp(ts, sizeof(ts));

    PCB child        = *parent;
    child.pid        = next_pid++;
    child.parent_pid = parent->pid;
    child.state      = NEW;
    child.creation_time = time(NULL);
    child.exit_code  = 0;

    pcb_table[pcb_count++] = child;

    /* transition NEW -> READY */
    pcb_table[pcb_count - 1].state = READY;

    printf("[%s] edu_fork: child PID=%d forked from parent PID=%d → state=READY\n",
           ts, child.pid, parent->pid);

    write_pcb_snapshot();
    return child.pid;
}

/* ── edu_exec ── */
void edu_exec(pid_t pid, char *prog_name, int burst, int remaining) {
    char ts[32];
    timestamp(ts, sizeof(ts));

    PCB *p = find_pcb(pid);
    if (!p) {
        fprintf(stderr, "[%s] edu_exec: PID=%d not found\n", ts, pid);
        return;
    }

    strncpy(p->name, prog_name, sizeof(p->name) - 1);
    p->name[sizeof(p->name) - 1] = '\0';
    p->burst_time     = burst;
    p->remaining_time = remaining;
    p->state          = RUNNING;

    printf("[%s] edu_exec: PID=%d image replaced with '%s' burst=%d → state=RUNNING\n",
           ts, pid, prog_name, burst);

    write_pcb_snapshot();
}

/* ── edu_wait ── */
int edu_wait(pid_t parent_pid) {
    char ts[32];
    timestamp(ts, sizeof(ts));

    printf("[%s] edu_wait: PID=%d waiting for children to terminate...\n",
           ts, parent_pid);

    /* Block until all children are TERMINATED */
    int waiting = 1;
    while (waiting) {
        waiting = 0;
        for (int i = 0; i < pcb_count; i++) {
            if (pcb_table[i].parent_pid == parent_pid &&
                pcb_table[i].state != TERMINATED) {
                waiting = 1;
                break;
            }
        }
        if (waiting) sleep(1);
    }

    /* Return last child's exit code */
    int exit_code = 0;
    for (int i = 0; i < pcb_count; i++)
        if (pcb_table[i].parent_pid == parent_pid)
            exit_code = pcb_table[i].exit_code;

    timestamp(ts, sizeof(ts));
    printf("[%s] edu_wait: PID=%d all children terminated, exit_code=%d\n",
           ts, parent_pid, exit_code);

    return exit_code;
}

/* ── edu_exit ── */
void edu_exit(pid_t pid, int exit_code) {
    char ts[32];
    timestamp(ts, sizeof(ts));

    PCB *p = find_pcb(pid);
    if (!p) {
        fprintf(stderr, "[%s] edu_exit: PID=%d not found\n", ts, pid);
        return;
    }

    p->state     = TERMINATED;
    p->exit_code = exit_code;

    printf("[%s] edu_exit: PID=%d terminated with exit_code=%d\n",
           ts, pid, exit_code);

    write_pcb_snapshot();
}

/* ── edu_ps ── */
void edu_ps(void) {
    printf("\n%-6s %-20s %-12s %-8s %-10s %-12s %-8s\n",
           "PID", "NAME", "STATE", "PRIORITY",
           "BURST", "REMAINING", "MEMORY");
    printf("%-6s %-20s %-12s %-8s %-10s %-12s %-8s\n",
           "------", "--------------------", "------------",
           "--------", "----------", "------------", "--------");

    for (int i = 0; i < pcb_count; i++) {
        PCB *p = &pcb_table[i];
        printf("%-6d %-20s %-12s %-8d %-10d %-12d %-8d\n",
               p->pid, p->name, state_name(p->state),
               p->priority, p->burst_time,
               p->remaining_time, p->memory_req_kb);
    }
    printf("\n");
}
