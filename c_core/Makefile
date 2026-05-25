CC = gcc
CFLAGS = -Wall -Wextra -pthread -std=c11
TARGET = eduos

SRCS = process_manager.c thread_manager.c ipc_module.c main_sim.c

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

race:
	$(CC) $(CFLAGS) -o race process_manager.c thread_manager.c

fixed:
	$(CC) $(CFLAGS) -o fixed process_manager.c thread_manager.c -D FIXED

memcheck:
	gcc $(CFLAGS) -o eduos $(SRCS)
	valgrind --leak-check=full ./eduos

clean:
	rm -f *.o $(TARGET) race fixed
