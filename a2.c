// driver code for s-talk

#include "a2.h"
#include "list.h"

pthread_mutex_t outgoingListMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t incomingListMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t socketMutex = PTHREAD_MUTEX_INITIALIZER;

List* outgoing_messages;
List* incoming_messages;

int serverSocket;
struct sockaddr_in serverAddress;

void init_serverSocket(const char* remoteMachine, int remotePort, int myPort) {
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
    serverAddress.sin_port = htons(myPort);  // Use your predefined SERVER_PORT

    // Set up remote server address
    memset(&remoteServerAddress, 0, sizeof(remoteServerAddress));
    remoteServerAddress.sin_family = AF_INET;
    remoteServerAddress.sin_addr.s_addr = inet_addr(remoteMachine);
    remoteServerAddress.sin_port = htons(remotePort);
    
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

        // Add message to the shared list
        pthread_mutex_lock(&outgoingListMutex);
        List_prepend(outgoing_messages, input);
        pthread_mutex_unlock(&outgoingListMutex);

        // Check for exit condition
        if (strcmp(input, "!\n\0") == 0) {
            pthread_mutex_lock(&incomingListMutex);
            List_prepend(incoming_messages, input);
            pthread_mutex_unlock(&incomingListMutex);
        }
    }

    pthread_exit(NULL);
}

void* udpSendFunction(void* arg) {

    while (1) {

        //get message from list
        pthread_mutex_lock(&outgoingListMutex);
        char* message = (char*)List_trim(outgoing_messages);
        pthread_mutex_unlock(&outgoingListMutex);

        // Send message to the remote client using UDP
        if (message) {
            pthread_mutex_lock(&socketMutex);
            sendto(serverSocket, message, strlen(message), 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
            pthread_mutex_unlock(&socketMutex);
            break;
        }

    }

    pthread_exit(NULL);
}

void* udpReceiveFunction(void* arg) {
    socklen_t serverAddressLen = sizeof(serverAddress);
    char buffer[MAX_MESSAGE_SIZE];

    while (1) {

        pthread_mutex_lock(&socketMutex);
        ssize_t bytesRead = recvfrom(serverSocket, buffer, MAX_MESSAGE_SIZE-1, 0, (struct sockaddr*)&serverAddress, &serverAddressLen);
        pthread_mutex_unlock(&socketMutex);

        if (bytesRead == -1) {
            perror("Error receiving message");
        }
        else {
            buffer[bytesRead] = '\0';
        
            pthread_mutex_lock(&incomingListMutex);
            List_prepend(incoming_messages, buffer);
            pthread_mutex_unlock(&incomingListMutex);
        }

    }

    pthread_exit(NULL);
}

void* screenOutputFunction(void* arg) {
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
                printf("Received: %s\n", message);
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

    int myPort = atoi(argv[1]);
    const char* remoteMachine = argv[2];
    int remotePort = atoi(argv[3]);

    // Create a socket for communication
    init_serverSocket(remoteMachine, remotePort, myPort);

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
