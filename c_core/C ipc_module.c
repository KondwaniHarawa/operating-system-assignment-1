#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

#include "include/eduos.h"

#define SHM_NAME "/eduos_shm"

// Shared memory structure
typedef struct {
    int pid;
    char message[128];
} SharedData;
