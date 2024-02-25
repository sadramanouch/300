// driver code for s-talk

#include "a2.h"
#include "list.h"

pthread_mutex_t outgoingListMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t outgoingListCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t incomingListMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t incomingListCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t socketMutex = PTHREAD_MUTEX_INITIALIZER;

List* outgoing_messages;
List* incoming_messages;

int serverSocket;
struct sockaddr_in localAddress;
struct sockaddr_in remoteAddress;

void init_serverSocket(const char* localPort, const char* remoteMachine, const char* remotePort) {
    // Create socket
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    memset(&localAddress, '\0', sizeof(localAddress));
    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(localPort);
    localAddress.sin_addr.s_addr = INADDR_ANY;

    // Set up remote server address
    memset(remoteAddress, '\0', sizeof(remoteAddress));
    remoteAddress.sin_family = AF_INET;
    remoteAddress.sin_port = htons(remotePort);
    remoteAddress.sin_addr.s_addr = inet_addr(remoteMachine);
    
    // Bind the socket
    if (bind(serverSocket, (struct sockaddr*)&localAddress, sizeof(struct sockaddr_in))) {
        perror("Error binding socket");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    sleep(5);
    char* message = "hi\n\0";
    send(serverSocket, message, strlen(message), 0);
    printf("sent msg\n");

    char buffer[MAX_MESSAGE_SIZE];
    recv(serverSocket, buffer, MAX_MESSAGE_SIZE, 0);
    printf("received msg: %s", buffer);

}

void* keyboardInputFunction(void* arg) {
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while (1) {
        char input[MAX_MESSAGE_SIZE];
        fgets(input, sizeof(input), stdin);

        // Add message to the shared list
        pthread_mutex_lock(&outgoingListMutex);
        List_prepend(outgoing_messages, input);
        pthread_mutex_unlock(&outgoingListMutex);

        // Check for exit condition
        if (strcmp(input, "!\n\0") == 0) {
            pthread_mutex_lock(&incomingListMutex);
            List_prepend(incoming_messages, input);
            pthread_mutex_unlock(&incomingListMutex);
            break;
        }
    }

    pthread_exit(NULL);
}

void* udpSendFunction(void* arg) {
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while (1) {

        //get message from list
        pthread_mutex_lock(&outgoingListMutex);
        char* message = (char*)List_trim(outgoing_messages);
        pthread_mutex_unlock(&outgoingListMutex);

        // Send message to the remote client using UDP
        if (message) {
            pthread_mutex_lock(&socketMutex);
            sendto(serverSocket, message, strlen(message), 0, (struct sockaddr*)&remoteAddress, sizeof(struct sockaddr_in));
            pthread_mutex_unlock(&socketMutex);
            break;
        }

    }

    pthread_exit(NULL);
}

void* udpReceiveFunction(void* arg) {
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    socklen_t serverAddressLen = sizeof(remoteAddress);
    char buffer[MAX_MESSAGE_SIZE];

    while (1) {

        pthread_mutex_lock(&socketMutex);
        ssize_t bytesRead = recvfrom(serverSocket, buffer, MAX_MESSAGE_SIZE, 0, (struct sockaddr*)&remoteAddress, &serverAddressLen);
        pthread_mutex_unlock(&socketMutex);

        if (bytesRead == -1) {
            perror("Error receiving message");
        }
        else {
        
            pthread_mutex_lock(&incomingListMutex);
            List_prepend(incoming_messages, buffer);
            pthread_mutex_unlock(&incomingListMutex);
        }

    }

    pthread_exit(NULL);
}

void* screenOutputFunction(void* arg) {
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while (1) {
        pthread_mutex_lock(&incomingListMutex);
        char* message = (char*)List_trim(incoming_messages);
        pthread_mutex_unlock(&incomingListMutex);

        // Check for exit condition
        if (message) {
            if (strcmp(message, "!\n\0") == 0) {
                break;
            }
            else {
                // change this use puts
                printf("Received: %s", message);
            }
        }

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

    const char* localPort = argv[1];
    const char* remoteMachine = argv[2];
    const char* remotePort = argv[3];

    printf("myport = %s\n", localPort);
    printf("remotemachine = %s\n", remoteMachine);
    printf("remoteport = %s\n", remotePort);

    // Create a socket for communication
    init_serverSocket(localPort, remoteMachine, remotePort);

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


    close(serverSocket);

    return 0;
}
