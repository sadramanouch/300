// a2.c

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

#define MAX_MESSAGE_SIZE 1024
#define SERVER_PORT 12345

struct List sharedList;

pthread_mutex_t listMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t listCond = PTHREAD_COND_INITIALIZER;

int programRunning = 1; // Global variable to control program execution

void initList(struct List* list) {
    list->head = NULL;
}

void pushMessage(struct List* list, const char* message) {
    pthread_mutex_lock(&listMutex);

    if (programRunning) {
        char* messageCopy = strdup(message);
        List_prepend(list, messageCopy);
        pthread_cond_signal(&listCond);
    }

    pthread_mutex_unlock(&listMutex);
}

void popMessage(struct List* list, char* message) {
    pthread_mutex_lock(&listMutex);

    while (programRunning && List_count(list) == 0) {
        pthread_cond_wait(&listCond, &listMutex);
    }

    if (programRunning) {
        char* poppedMessage = (char*)List_trim(list);
        strncpy(message, poppedMessage, MAX_MESSAGE_SIZE);
        free(poppedMessage);
    }

    pthread_mutex_unlock(&listMutex);
}

int createSocket() {
    int serverSocket;
    struct sockaddr_in serverAddress;

    //Create socket
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    //Set up server address
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(SERVER_PORT);

    //Bind the socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Error binding socket");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    return serverSocket;
}

void sendMessage(const char* ipAddress, int port, const char* message) {
    int clientSocket;
    struct sockaddr_in clientAddress;

    //Create socket
    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    //Set up client address
    memset(&clientAddress, 0, sizeof(clientAddress));
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_addr.s_addr = inet_addr(ipAddress);
    clientAddress.sin_port = htons(port);

    //Send the message
    sendto(clientSocket, message, strlen(message), 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));

    close(clientSocket);
}

void receiveMessage(int serverSocket, char* buffer) {
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLen = sizeof(clientAddress);

    //Receive message
    ssize_t bytesRead = recvfrom(serverSocket, buffer, MAX_MESSAGE_SIZE, 0, (struct sockaddr*)&clientAddress, &clientAddressLen);
    if (bytesRead == -1) {
        perror("Error receiving message");
    }

    //Null-terminate the received message
    buffer[bytesRead] = '\0';
}

void* keyboardInputThread(void* arg) {
    char input[MAX_MESSAGE_SIZE];

    while (1) {
        printf("Type a message: ");
        fgets(input, sizeof(input), stdin);

        // Check for exit condition
        if (strcmp(input, "exit\n") == 0) {
            pthread_mutex_lock(&listMutex);
            programRunning = 0; // Set programRunning to 0 to signal threads to exit
            pthread_cond_broadcast(&listCond); // Signal all threads to wake up
            pthread_mutex_unlock(&listMutex);
            break;
        }

        // Remove newline character
        input[strcspn(input, "\n")] = '\0';

        // Add message to the shared list
        pushMessage(&sharedList, input);
    }

    pthread_exit(NULL);
}

void* udpOutputThread(void* arg) {
    int remotePort = *((int*)arg);

    while (1) {
        char message[MAX_MESSAGE_SIZE];
        //Get message from the shared list
        popMessage(&sharedList, message);

        //Send message to the remote client using UDP
        sendMessage("127.0.0.1", remotePort, message);
    }

    pthread_exit(NULL);
    return NULL;
}

void* udpOutputThread(void* arg) {
    int remotePort = *((int*)arg);

    while (1) {
        char message[MAX_MESSAGE_SIZE];
        //Get message from the shared list
        popMessage(&sharedList, message);

        //Send message to the remote client using UDP
        sendMessage("127.0.0.1", remotePort, message);
    }

    pthread_exit(NULL);
}

void* screenOutputThread(void* arg) {
    while (1) {
        char message[MAX_MESSAGE_SIZE];
        //Get message from the shared list
        popMessage(&sharedList, message);

        //Print message to the local screen
        printf("Received: %s\n", message);
    }

    pthread_exit(NULL);
    return NULL;
}

void* mainThread(void* arg) {
    //Initialization code
    initList(&sharedList);

    //Create a socket for communication
    int serverSocket = createSocket();

    //Start threads
    pthread_t keyboardThread, udpOutputThread, udpInputThread, screenOutputThread;
    pthread_create(&keyboardThread, NULL, keyboardInputThread, NULL);
    pthread_create(&udpOutputThread, NULL, udpOutputThread, &SERVER_PORT);
    pthread_create(&udpInputThread, NULL, udpInputThread, NULL);
    pthread_create(&screenOutputThread, NULL, screenOutputThread, NULL);

    //Wait for threads to finish
    pthread_join(keyboardThread, NULL);
    pthread_join(udpOutputThread, NULL);
    pthread_join(udpInputThread, NULL);
    pthread_join(screenOutputThread, NULL);

    close(serverSocket);

    return NULL;
}

int main() {
    pthread_t mainThreadId;
    pthread_create(&mainThreadId, NULL, mainThread, NULL);
    pthread_join(mainThreadId, NULL);

    return 0;
}