#include "include/eduos.h"

extern PCB pcb_table[100];
extern int pcb_count;
extern pid_t edu_fork(PCB *parent);

int main()
{
    PCB p1;

    p1.pid = 0;
    strcpy(p1.name, "InitProcess");
    p1.state = 0;

    edu_fork(&p1);

    printf("Total processes: %d\n", pcb_count);

    return 0;
}
