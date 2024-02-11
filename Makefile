CC = gcc
CFLAGS = -Wall -Wextra -pthread  
TARGET = a2  

all: $(TARGET)

$(TARGET): a2.o main.o
	$(CC) $(CFLAGS) -o $(TARGET) a2.o main.o

a2.o: a2.c a2.h
	$(CC) $(CFLAGS) -c a2.c

main.o: main.c a2.h
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f $(TARGET) *.o
