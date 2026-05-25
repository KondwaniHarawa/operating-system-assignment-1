PCB pcb_table[100];
int pcb_count = 0;
int next_pid = 1;

pid_t edu_fork(PCB *parent)
{
    PCB child = *parent;

    child.pid = next_pid++;
    child.state = 1;

    pcb_table[pcb_count++] = child;

    printf("[INFO] Forked process PID=%d\n", child.pid);

    return child.pid;
}
