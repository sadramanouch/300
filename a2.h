#ifndef A2_H
#define A2_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

//Define constants
#define MAX_MESSAGE_SIZE 1024
#define SERVER_PORT 12345

//Define the list ADT and related synchronization primitives
struct Node{
    char message[MAX_MESSAGE_SIZE];
    struct Node* next;
};

struct List{
    struct Node* head;
};

//Mutex and condition variable for synchronization
extern pthread_mutex_t listMutex;
extern pthread_cond_t listCond;

//Shared list of messages
extern struct List sharedList;

//Function prototypes
void initList(struct List* list);
void pushMessage(struct List* list, const char* message);
void popMessage(struct List* list, char* message);

//Socket-related functions
int createSocket();
void sendMessage(const char* ipAddress, int port, const char* message);
void receiveMessage(int serverSocket, char* buffer);

//Thread functions
void* keyboardInputThread(void* arg);
void* udpOutputThread(void* arg);
void* udpInputThread(void* arg);
void* screenOutputThread(void* arg);

#endif //A2_H