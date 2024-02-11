#include "a2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

//Define constants
#define MAX_MESSAGE_SIZE 1024
#define SERVER_PORT 12345

//Define the list ADT and related synchronization primitives
//(Note: You should implement your list ADT from Assignment #1)

//Example List ADT structure
struct List {
    // Add your list implementation here
};

//Mutex and condition variable for synchronization
pthread_mutex_t listMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t listCond = PTHREAD_COND_INITIALIZER;

//Shared list of messages
struct List sharedList;

//Function prototypes
void* keyboardInputThread(void* arg);
void* udpOutputThread(void* arg);
void* udpInputThread(void* arg);
void* screenOutputThread(void* arg);

int main(int argc, char* argv[]) {
    //Parse command line arguments to get port and remote machine information
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <my port number> <remote machine name> <remote port number>\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    int myPort = atoi(argv[1]);
    const char* remoteMachine = argv[2];
    int remotePort = atoi(argv[3]);

    //Initialize the shared list
    //(Note: You should initialize your list ADT based on Assignment #1)

    //Initialize threads and sockets
    pthread_t keyboardThread,udpOutputThread,udpInputThread,screenThread;

    //Create threads
    pthread_create(&keyboardThread,NULL,keyboardInputThread,NULL);
    pthread_create(&udpOutputThread,NULL,udpOutputThread,(void*)&remotePort);
    pthread_create(&udpInputThread,NULL,udpInputThread,NULL);
    pthread_create(&screenThread,NULL,screenOutputThread,NULL);

    //Wait for threads to finish
    pthread_join(keyboardThread,NULL);
    pthread_join(udpOutputThread,NULL);
    pthread_join(udpInputThread,NULL);
    pthread_join(screenThread,NULL);

    return 0;
}