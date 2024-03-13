#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"


#define MAX_PROCESSES 100
#define MAX_MESSAGE_LENGTH 40

typedef enum { HIGH, NORMAL, LOW } Priority;
typedef enum { READY, BLOCKED, RUNNING, TERMINATED } Status;

typedef struct {
    int pid;
    Priority priority;
    Status status;
    char proc_message[MAX_MESSAGE_LENGTH];

} PCB;

typedef struct {
    PCB processes[MAX_PROCESSES];
    int process_count;
    List* queues[3];  // Low, Medium, High priority queues

} OS;

List lists[LIST_MAX_NUM_HEADS];
Node nodes[LIST_MAX_NUM_NODES];

void init(OS *os) {
    // Initialize the operating system
    os->process_count = 0;

    for (int i = 0; i < 3; i++) {
        os->queues[i] = List_create();
        if (!os->queues[i]) {
            printf("Error: Unable to create priority queue %d.\n", i);
            exit(EXIT_FAILURE);
        }
    }
}

int create(OS *os, Priority priority) {

    if (os->process_count >= MAX_PROCESSES) {
        printf("Error: Maximum number of processes reached.\n");
        return -1; 
    }

    PCB new_process;
    new_process.pid = (intptr_t)&new_process; //using intptr_t for pointer to integer
    new_process.priority = priority;
    new_process.status = READY;

    List* newQueue = os->queues[priority];
    if (!newQueue) {
        printf("Error: Unable to access priority queue %d.\n", priority);
        return -1; 
    }
    List_append(newQueue, &new_process);

    os->processes[os->process_count++] = new_process;

    printf("Process created with PID: %d\n", (void*)(intptr_t)new_process.pid);
    return new_process.pid; //success
}

// Function to fork a process (create a copy) and add it to the ready queue
void fork(OS *os) {
    // Implement Fork logic here
}

// Function to kill the specified process and remove it from the system
void kill(OS *os, int pid) {
    // Implement Kill logic here
}

// Function to exit the currently running process
void exit(OS *os) {
    // Implement Exit logic here
}

// Function to handle the quantum expiry (time quantum of the running process)
void quantum(OS *os) {
    // Implement Quantum expiry logic here
}

// Function to send a message to another process and block until a reply is received
void send(OS *os, int target_pid, char *msg) {
    // Implement send_message logic here
}

// Function to receive a message and block until one arrives
void receive(OS *os) {
    // Implement receive_message logic here
}

// Function to make a reply to a process and unblock the sender
void reply(OS *os, int reply_pid, char *reply_msg) {
    // Implement reply logic here
}

// Function to initialize a semaphore with the given ID and initial value
void semaphore(OS *os, int semaphore_id, int initial_value) {
    // Implement initialize_semaphore logic here
}

// Function to perform the P operation on a semaphore
void semaphore_P(OS *os, int semaphore_id) {
    // Implement semaphore_P_operation logic here
}

// Function to perform the V operation on a semaphore
void semaphore_V(OS *os, int semaphore_id) {
    // Implement semaphore_V_operation logic here
}

// Function to display complete state information of a process
void process_info(OS *os, int pid) {
    // Implement process_info logic here
}

// Function to display all process queues and their contents
void total_info(OS *os) {
    // Implement total_info logic here
}

int main() {
    OS os;
    init(&os);

    printf("Interactive Operating System Simulation\n");

    char command;
    do {
        printf("\nEnter a command (C, F, K, E, Q, S, R, Y, N, P, V, I, T): ");
        scanf(" %c", &command);

        switch (command) {
            case 'C': {
                int priority;
                printf("Enter priority (0 = high, 1 = norm, 2 = low): ");
                scanf("%d", &priority);
                create(&os, priority);
                break;
            }
            case 'F':
                printf("Forking the currently running process...\n");
                fork(&os);
                break;
            case 'K': {
                int pid;
                printf("Enter PID of the process to be killed: ");
                scanf("%d", &pid);
                kill(&os, pid);
                break;
            }
            case 'E':
                printf("Exiting the currently running process...\n");
                exit(&os);
                break;
            case 'Q':
                printf("Time quantum of the running process expired...\n");
                quantum(&os);
                break;
            case 'S': {
                int target_pid;
                char message[MAX_MESSAGE_LENGTH];
                printf("Enter PID of the process to send message to: ");
                scanf("%d", &target_pid);
                printf("Enter message (max 40 characters): ");
                scanf("%s", message);
                send(&os, target_pid, message);
                break;
            }
            case 'R':
                printf("Waiting to receive a message...\n");
                recieve(&os);
                break;
            case 'Y': {
                int reply_pid;
                char reply_message[MAX_MESSAGE_LENGTH];
                printf("Enter PID of the process to make the reply to: ");
                scanf("%d", &reply_pid);
                printf("Enter reply message (max 40 characters): ");
                scanf("%s", reply_message);
                reply(&os, reply_pid, reply_message);
                break;
            }
            case 'N': {
                int semaphore_id, initial_value;
                printf("Enter semaphore ID (0-4): ");
                scanf("%d", &semaphore_id);
                printf("Enter initial value: ");
                scanf("%d", &initial_value);
                semaphor(&os, semaphore_id, initial_value);
                break;
            }
            case 'P': {
                int semaphore_id;
                printf("Enter semaphore ID (0-4) for P operation: ");
                scanf("%d", &semaphore_id);
                semaphor_P(&os, semaphore_id);
                break;
            }
            case 'V': {
                int semaphore_id;
                printf("Enter semaphore ID (0-4) for V operation: ");
                scanf("%d", &semaphore_id);
                semaphore_V(&os, semaphore_id);
                break;
            }
            case 'I': {
                int pid;
                printf("Enter PID of the process for which information is to be returned: ");
                scanf("%d", &pid);
                process_info(&os, pid);
                break;
            }
            case 'T':
                printf("Displaying all process queues and their contents...\n");
                total_info(&os);
                break;
            default:
                printf("Invalid command. Try again.\n");
        }

    } while (command != 'X');

    return 0;
}

