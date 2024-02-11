#include "a2.h"
#include "list.h"  //This is where the list implementation that he give us goes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_MESSAGE_SIZE 1024
#define SERVER_PORT 12345

struct Node{
    char message[MAX_MESSAGE_SIZE];
    struct Node* next;
};

struct List{
    struct Node* head;
};

struct List sharedList;

pthread_mutex_t listMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t listCond = PTHREAD_COND_INITIALIZER;

void initList(struct List* list){
    list->head = NULL;
}

void pushMessage(struct List* list, const char* message){
    pthread_mutex_lock(&listMutex);

    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    strncpy(newNode->message,message,MAX_MESSAGE_SIZE);
    newNode->next = list->head;
    list->head = newNode;

    pthread_cond_signal(&listCond);
    pthread_mutex_unlock(&listMutex);
}

void popMessage(struct List* list, char* message){
    pthread_mutex_lock(&listMutex);

    while (list->head == NULL){
        pthread_cond_wait(&listCond,&listMutex);
    }

    struct Node* temp = list->head;
    list->head = temp->next;
    strncpy(message,temp->message,MAX_MESSAGE_SIZE);
    free(temp);

    pthread_mutex_unlock(&listMutex);
}

void* keyboardInputThread(void* arg){
    char input[MAX_MESSAGE_SIZE];

    while (1) {
        printf("Type a message: ");
        fgets(input, sizeof(input),stdin);

        //Check for exit condition
        if (strcmp(input,"exit\n") == 0){
            break;
        }

        //Remove newline character
        input[strcspn(input,"\n")] ='\0';

        //Add message to the shared list
        pushMessage(&sharedList,input);
    }

    pthread_exit(NULL);
}

void* udpOutputThread(void* arg){
    int remotePort = *((int*)arg);

    while (1) {
        char message[MAX_MESSAGE_SIZE];
        //Get message from the shared list
        popMessage(&sharedList,message);

        //Send message to the remote client using UDP
        sendMessage("127.0.0.1",remotePort,message);
    }

    pthread_exit(NULL);
}

void* udpInputThread(void* arg){
    int serverSocket = createSocket();

    while (1){
        char buffer[MAX_MESSAGE_SIZE];
        receiveMessage(serverSocket,buffer);
        //Add received message to the shared list
        pushMessage(&sharedList,buffer);
    }

    close(serverSocket);
    pthread_exit(NULL);
}

void* screenOutputThread(void* arg){
    while (1){
        char message[MAX_MESSAGE_SIZE];
        //Get message from the shared list
        popMessage(&sharedList,message);

        //Print message to the local screen
        printf("Received: %s\n",message);
    }

    pthread_exit(NULL);
}

int main(){
    //Initialization code
    initList(&sharedList);

    //Create a socket for communication
    int serverSocket = createSocket();

    //Start threads
    pthread_t keyboardThread, udpOutputThread, udpInputThread, screenOutputThread;
    pthread_create(&keyboardThread,NULL,keyboardInputThread,NULL);
    pthread_create(&udpOutputThread,NULL,udpOutputThread,(void*)&SERVER_PORT);
    pthread_create(&udpInputThread,NULL, udpInputThread,NULL);
    pthread_create(&screenOutputThread,NULL, screenOutputThread,NULL);

    //Wait for threads to finish
    pthread_join(keyboardThread,NULL);
    pthread_join(udpOutputThread,NULL);
    pthread_join(udpInputThread,NULL);
    pthread_join(screenOutputThread,NULL);

    close(serverSocket);

    return 0;
}