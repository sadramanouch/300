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

pthread_mutex_t outgoingListMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t outgoingListCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t incomingListMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t incomingListCond = PTHREAD_COND_INITIALIZER;

List* outgoing_messages;
List* incoming_messages;

int serverSocket;
struct sockaddr_in serverAddress;

int programRunning = 1; // Global variable to control program execution

void init_serverSocket() {
    // Define SERVER_PORT if not already defined
    #define SERVER_PORT 12345

    // Create socket
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(SERVER_PORT);

    // Bind the socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Error binding socket");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }
}

void* keyboardInputFunction(void* arg) {
    char input[MAX_MESSAGE_SIZE];

    while (1) {
        printf("You: ");
        fgets(input, sizeof(input), stdin);

        // Check for exit condition
        if (strcmp(input, "!\n") == 0) {
            pthread_mutex_lock(&outgoingListMutex);
            programRunning = 0; // Set programRunning to 0 to signal threads to exit
            pthread_cond_broadcast(&outgoingListCond); // Signal all threads to wake up
            pthread_mutex_unlock(&outgoingListMutex);
            break;
        }

        // Remove newline character
        input[strcspn(input, "\n")] = '\0';

        // Add message to the shared list
        pthread_mutex_lock(&outgoingListMutex);
        List_append(outgoing_messages, input);
        pthread_cond_signal(&outgoingListCond); // Signal one thread to wake up
        pthread_mutex_unlock(&outgoingListMutex);
    }

    pthread_exit(NULL);
}

void* udpSendFunction(void* arg) {
    while (1) {
        pthread_mutex_lock(&outgoingListMutex);
        while (List_count(outgoing_messages) == 0 && programRunning) {
            pthread_cond_wait(&outgoingListCond, &outgoingListMutex);
        }

        if (!programRunning) {
            pthread_mutex_unlock(&outgoingListMutex);
            break;
        }

        char* message = (char*)List_trim(outgoing_messages);
        pthread_mutex_unlock(&outgoingListMutex);

        // Send message to the remote client using UDP
        sendto(serverSocket, message, strlen(message), 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    }

    pthread_exit(NULL);
}

void* udpReceiveFunction(void* arg) {
    socklen_t serverAddressLen = sizeof(serverAddress);
    char buffer[MAX_MESSAGE_SIZE];

    while (1) {
        ssize_t bytesRead = recvfrom(serverSocket, buffer, MAX_MESSAGE_SIZE, 0, (struct sockaddr*)&serverAddress, &serverAddressLen);
        if (bytesRead == -1) {
            perror("Error receiving message");
        }

        buffer[bytesRead] = '\0';
        
        pthread_mutex_lock(&incomingListMutex);
        List_append(incoming_messages, buffer);
        pthread_cond_signal(&incomingListCond);
        pthread_mutex_unlock(&incomingListMutex);
    }

    pthread_exit(NULL);
}

void* screenOutputFunction(void* arg) {
    while (1) {
        pthread_mutex_lock(&incomingListMutex);
        while (List_count(incoming_messages) == 0 && programRunning) {
            pthread_cond_wait(&incomingListCond, &incomingListMutex);
        }

        if (!programRunning) {
            pthread_mutex_unlock(&incomingListMutex);
            break;
        }

        char message[MAX_MESSAGE_SIZE];
        strcpy(message, (char*)List_trim(incoming_messages));
        pthread_mutex_unlock(&incomingListMutex);

        printf("Received: %s\n", message);
    }

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    // user input for ports and machine name
    if (argc != 4) {
        printf("Wrong number of arguments. Please enter:\n ./s-talk [your port] [friend's machine] [friend's port]\n");
        return -1;
    }

    // Initialization of lists
    outgoing_messages = List_create();
    incoming_messages = List_create();

    // Create a socket for communication
    init_serverSocket();

    // Start threads
    pthread_t keyboardInputThread, udpSendThread, udpReceiveThread, screenOutputThread;
    pthread_create(&keyboardInputThread, NULL, keyboardInputFunction, NULL);
    pthread_create(&udpSendThread, NULL, udpSendFunction, NULL);
    pthread_create(&udpReceiveThread, NULL, udpReceiveFunction, NULL);
    pthread_create(&screenOutputThread, NULL, screenOutputFunction, NULL);

    // Wait for exit signal
    pthread_join(keyboardInputThread, NULL);

    // Cancel and join the other threads
    pthread_cancel(udpSendThread);
    pthread_cancel(udpReceiveThread);
    pthread_cancel(screenOutputThread);

    pthread_join(udpSendThread, NULL);
    pthread_join(udpReceiveThread, NULL);
    pthread_join(screenOutputThread, NULL);

    close(serverSocket);

    return 0;
}
