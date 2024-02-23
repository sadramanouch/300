#ifndef A2_H
#define A2_H

#include "a2.h"
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

//Define constants
#define MAX_MESSAGE_SIZE 256

//Mutex and condition variable for synchronization
extern pthread_mutex_t outgoinglistMutex;
extern pthread_cond_t outgoinglistCond;
extern pthread_mutex_t incominglistMutex;
extern pthread_cond_t incominglistCond;

//Socket-related functions
void init_serverSocket();
void receiveMessage(int serverSocket, char* buffer);

//Thread functions
void* keyboardInputFunction(void* arg);
void* udpSendFunction(void* arg);
void* udpReceiveFunction(void* arg);
void* screenOutputFunction(void* arg);

#endif //A2_H