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

}

void receiveMessage(int serverSocket, char* buffer) {
    socklen_t serverAddressLen = sizeof(serverAddress);

    //Receive message
    ssize_t bytesRead = recvfrom(serverSocket, buffer, MAX_MESSAGE_SIZE, 0, (struct sockaddr*)&serverAddress, &serverAddressLen);
    if (bytesRead == -1) {
        perror("Error receiving message");
    }

    //Null-terminate the received message
    buffer[bytesRead] = '\0';
}

void* keyboardInputFunction(void* arg) {
    char input[MAX_MESSAGE_SIZE];

    while (1) {
        printf("You: ");
        fgets(input, sizeof(input), stdin);

        // Check for exit condition
        if (strcmp(input, "!\n") == 0) {
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

void* udpSendFunction(void* arg) {
    int remotePort = atoi((char*)arg[3]);
    char* remoteMachine = (char*)*arg[2];

    while (1) {
        char* message = (char*)List_trim(outgoing_messages);

        //Send message to the remote client using UDP
        sendto(serverSocket, message, strlen(message), 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    }

    pthread_exit(NULL);
    return NULL;
}

void* udpReceiveFunction(void* arg) {
    socklen_t serverAddressLen = sizeof(serverAddress);
    char buffer[MAX_MESSAGE_SIZE];

    //Receive message
    ssize_t bytesRead = recvfrom(serverSocket, buffer, MAX_MESSAGE_SIZE, 0, (struct sockaddr*)&serverAddress, &serverAddressLen);
    if (bytesRead == -1) {
        perror("Error receiving message");
    }

    //Null-terminate the received message
    buffer[bytesRead] = '\0';

    return NULL;
}

void* screenOutputFunction(void* arg) {
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

int main(int argc, char* argv[]) {
    //user input for ports and machine name
    if (argc != 4) {
        printf("Wrong number of arguments. Please enter:\n ./s-talk [your port] [friend's machine] [friend's port]\n");
        return -1;
    }

    //char* myPort = *argv[1];            //argv[0] will be ./s-talk
    //char* friendsMachine = *argv[2];
    //char* friendsPort = *argv[3];

    //Initialization of lists
    outgoing_messages = List_create();
    incoming_messages = List_create();

    //Create a socket for communication
    init_serverSocket();

    //Start threads
    pthread_t keyboardInputThread, udpSendThread, udpReceiveThread, screenOutputThread;
    pthread_create(&keyboardInputThread, NULL, keyboardInputFunction, NULL);
    pthread_create(&udpSendThread, NULL, udpSendFunction, (void*)argv);
    pthread_create(&udpReceiveThread, NULL, udpReceiveFunction, (void*)argv);
    pthread_create(&screenOutputThread, NULL, screenOutputFunction, NULL);

    //Wait for exit signal
    pthread_join(keyboardInputThread, NULL);

    //cancel and join the other threads
    pthread_cancel(udpSendThread)
    pthread_cancel(udpReceiveThread)
    pthread_cancel(screenOutputThread)
    pthread_join(udpSendThread, NULL);
    pthread_join(udpReceiveThread, NULL);
    pthread_join(screenOutputThread, NULL);

    close(serverSocket);

    return 0;
}