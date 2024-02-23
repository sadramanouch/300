CC = gcc
CFLAGS = -Wall -Wextra -pthread

all: a2

a2: a2.o list.o
	$(CC) $(CFLAGS) -o a2 a2.o list.o

a2.o: a2.c
	$(CC) $(CFLAGS) -c a2.c

list.o: list.c list.h
	$(CC) $(CFLAGS) -c list.c

clean:
	rm -f a2 *.o
