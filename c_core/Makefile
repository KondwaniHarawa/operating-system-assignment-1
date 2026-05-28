CC      = gcc
CFLAGS  = -Wall -Wextra -pthread -std=c11
TARGET  = eduos
SRCS    = process_manager.c thread_manager.c ipc_module.c main_sim.c

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -lrt

race:
	$(CC) $(CFLAGS) -DRACE -o race process_manager.c thread_manager.c ipc_module.c main_sim.c -lrt

fixed:
	$(CC) $(CFLAGS) -DFIXED -o fixed process_manager.c thread_manager.c ipc_module.c main_sim.c -lrt

memcheck:
	$(CC) $(CFLAGS) -g -o $(TARGET) $(SRCS) -lrt
	valgrind --leak-check=full --track-origins=yes ./$(TARGET)

clean:
	rm -f *.o $(TARGET) race fixed pcb_snapshot.json
