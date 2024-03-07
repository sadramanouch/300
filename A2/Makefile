# CC = gcc
# CFLAGS = -Wall -Wextra -pthread

# all: s-talk

# s-talk: a2.o list.o
# 	$(CC) $(CFLAGS) -o s-talk a2.o list.o

# a2.o: a2.c
# 	$(CC) $(CFLAGS) -c a2.c

# list.o: list.c list.h
# 	$(CC) $(CFLAGS) -c list.c

# clean:
# 	rm -f s-talk *.o


CC = gcc
CFLAGS = -Wall -Wextra -pthread

CFLAGS += -DDEBUG

all: s-talk

s-talk: a2.o list.o
	$(CC) $(CFLAGS) -o s-talk a2.o list.o

a2.o: a2.c
	$(CC) $(CFLAGS) -c a2.c

list.o: list.c list.h
	$(CC) $(CFLAGS) -c list.c

clean:
	rm -f s-talk *.o