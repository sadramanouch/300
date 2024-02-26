#ifndef A2_H
#define A2_H

//Define constants
#define MAX_MESSAGE_SIZE 256

//Thread functions
void* keyboardInputFunction(void* arg);
void* udpSendFunction(void* arg);
void* udpReceiveFunction(void* arg);
void* screenOutputFunction(void* arg);

//List functions
void freeItem(void* pItem);

#endif //A2_H