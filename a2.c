// driver code for s-talk

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
#include <netdb.h>
#include <sys/types.h>

pthread_mutex_t outgoingListMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t outgoingListCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t incomingListMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t incomingListCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t socketMutex = PTHREAD_MUTEX_INITIALIZER;

List* outgoing_messages;
List* incoming_messages;

struct sockaddr_in localAddress;
static struct addrinfo* res;
struct addrinfo hints;

char* localPort;
char* remoteMachine;
char* remotePort;

void* keyboardInputFunction(void* arg) {
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while (1) {
        char* input = malloc(MAX_MESSAGE_SIZE * sizeof(char));
        fgets(input, MAX_MESSAGE_SIZE, stdin);

        // Add message to the shared list
        pthread_mutex_lock(&outgoingListMutex);
        List_prepend(outgoing_messages, input);
        pthread_cond_signal(&outgoingListCond);
        pthread_mutex_unlock(&outgoingListMutex);

        // Check exit condition, shutdown happens from screenOutputFunction on both clients
        if (strcmp(input, "!\n\0") == 0) {
            pthread_mutex_lock(&incomingListMutex);
            List_prepend(incoming_messages, input);
            pthread_cond_signal(&incomingListCond);
            pthread_mutex_unlock(&incomingListMutex);
            break;
        }
    }

    pthread_exit(NULL);
}

void* udpSendFunction(void* arg) {

    int sendSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (sendSocket < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }
    
    //sending requirements
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    int addressStatus = getaddrinfo(remoteMachine, remotePort, &hints, &res);
    if (addressStatus != 0) {
        perror("Address or port error");
        exit(EXIT_FAILURE);
    }

    while (1) {

        //get message from list
        pthread_mutex_lock(&outgoingListMutex);
        while (List_count(outgoing_messages) == 0) {
            pthread_cond_wait(&outgoingListCond, &outgoingListMutex);
        }
        char* message = (char*)List_trim(outgoing_messages);
        pthread_mutex_unlock(&outgoingListMutex);

        // Send message to the remote client using UDP
        if (message) {
            sendto(sendSocket, message, strlen(message), 0, (struct sockaddr*)res->ai_addr, res->ai_addrlen);
            free(message);
        }
        if (strcmp(message, "!\n") == 0) {
            free(message);
            break;
        }
    }
    close(sendSocket);
    pthread_exit(NULL);
}

void* udpReceiveFunction(void* arg) {
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    int serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    //receiveing requirements
    memset(&localAddress, 0, sizeof(localAddress));
    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(atoi(localPort));
    localAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(serverSocket, (struct sockaddr*)&localAddress, sizeof(struct sockaddr_in)) < 0) {
        perror("Error binding socket");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in remoteAddress;
    socklen_t addressLength = sizeof(remoteAddress);

    while (1) {
        char* buffer = malloc(MAX_MESSAGE_SIZE * sizeof(char));

        int bytesRead = recvfrom(serverSocket, buffer, MAX_MESSAGE_SIZE-1, 0, (struct sockaddr*)&remoteAddress, &addressLength);

        if (bytesRead == -1) {
            perror("Error receiving message");
        }
        else {
            buffer[bytesRead] = '\0'; //in case the null character was lost in transmission
        
            pthread_mutex_lock(&incomingListMutex);
            List_prepend(incoming_messages, buffer);
            pthread_cond_signal(&incomingListCond);
            pthread_mutex_unlock(&incomingListMutex);
        }

        if (strcmp(buffer, "!\n") == 0) {
            break;
        }
    }
    close(serverSocket);
    pthread_exit(NULL);
}

void* screenOutputFunction(void* arg) {
    _Bool shutdown = 0;

    while (!shutdown) {

        pthread_mutex_lock(&incomingListMutex);
        while (List_count(incoming_messages) == 0) {
            pthread_cond_wait(&incomingListCond, &incomingListMutex);
        }
        char* message = (char*)List_trim(incoming_messages);
        pthread_mutex_unlock(&incomingListMutex);

        if (message) {

            printf("Received: ");
            fputs(message, stdout);

            if (strcmp(message, "!\n\0") == 0) { //check for exit condition
                shutdown = 1;
            }
            free(message);
        }

    }

    pthread_exit(NULL);
}

void freeItem(void* pItem) {
    free((char*)pItem);
}

void cleanup() {
    // Destroy mutex and condition variables
    pthread_mutex_destroy(&outgoingListMutex);
    pthread_mutex_destroy(&incomingListMutex);
    pthread_mutex_destroy(&socketMutex);
    pthread_cond_destroy(&outgoingListCond);
    pthread_cond_destroy(&incomingListCond);

    // Free dynamically allocated memory
    freeaddrinfo(res);
    List_free(outgoing_messages, freeItem);
    List_free(incoming_messages, freeItem);
}

int main(int argc, char* argv[]) {
    // user input for ports and machine name
    if (argc != 4) {
        printf("Wrong number of arguments. Please enter:\n ./s-talk [your port] [friend's machine/address] [friend's port]\n");
        return -1;
    }

    // Initialization of lists
    outgoing_messages = List_create();
    incoming_messages = List_create();

    localPort = argv[1];
    remoteMachine = argv[2];
    remotePort = argv[3];

    printf("YOUR PORT: %s\n", localPort);
    printf("REMOTE ADDRESS = %s\n", remoteMachine);
    printf("REMOTE PORT = %s\n\n", remotePort);

    // Start threads
    pthread_t keyboardInputThread, udpSendThread, udpReceiveThread, screenOutputThread;
    pthread_create(&keyboardInputThread, NULL, keyboardInputFunction, NULL);
    pthread_create(&udpSendThread, NULL, udpSendFunction, NULL);
    pthread_create(&udpReceiveThread, NULL, udpReceiveFunction, NULL);
    pthread_create(&screenOutputThread, NULL, screenOutputFunction, NULL);

    // Wait for exit signal
    pthread_join(screenOutputThread, NULL);

    // Cancel and join the other threads
    pthread_cancel(keyboardInputThread);
    pthread_cancel(udpSendThread);
    pthread_cancel(udpReceiveThread);
    
    pthread_join(keyboardInputThread, NULL);
    pthread_join(udpSendThread, NULL);
    pthread_join(udpReceiveThread, NULL);

    //clean up
    //close(serverSocket);
    cleanup();

    return 0;
}
