CC = gcc
CFLAGS = -Wall
#CFLAGS = -Wall -DDEBUG

#OBJ = ppos_core.o pingpong-mqueue.o queue.o -lm
#OBJ = ppos_core.o pingpong-prodcons.o queue.o
#OBJ = ppos_core.o pingpong-racecond.o queue.o
OBJ = ppos_core.o pingpong-semaphore.o queue.o


all: $(OBJ)
	$(CC) $(CFLAGS) -o ppos $(OBJ) && rm *.o

debug: $(OBJ)
	$(CC) $(CFLAGS) -DDEBUG -o ppos $(OBJ) && rm *.o

clean: $(OBJ)
	rm *.o