CC      = gcc
CFLAGS  = -Wall -Wextra -pthread -std=c11
TARGET  = eduos
SRCS    = process_manager.c thread_manager.c ipc_module.c main_sim.c

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -lrt

race:
	$(CC) $(CFLAGS) -DRACE -o race_demo thread_manager.c process_manager.c ipc_module.c main_sim.c -lrt
	@echo "=== Run 1 ===" && ./race_demo 2>&1 | grep "RACE\|FIXED"
	@echo "=== Run 2 ===" && ./race_demo 2>&1 | grep "RACE\|FIXED"
	@echo "=== Run 3 ===" && ./race_demo 2>&1 | grep "RACE\|FIXED"
	@echo "=== Run 4 ===" && ./race_demo 2>&1 | grep "RACE\|FIXED"
	@echo "=== Run 5 ===" && ./race_demo 2>&1 | grep "RACE\|FIXED"

fixed:
	$(CC) $(CFLAGS) -DFIXED -o fixed_demo thread_manager.c process_manager.c ipc_module.c main_sim.c -lrt
	./fixed_demo 2>&1 | grep "FIXED"

memcheck:
	$(CC) $(CFLAGS) -g -o $(TARGET) $(SRCS) -lrt
	valgrind --leak-check=full --track-origins=yes ./$(TARGET)

clean:
	rm -f *.o $(TARGET) race_demo fixed_demo pcb_snapshot.json
