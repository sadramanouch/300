#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "OS.h"



void init(OS *os) {
    //Initialize the operating system

    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        Sem* newSem;
        os->semaphores[i] = newSem;
        os->semaphore_wait_queues[i] = List_create();
    }

    for (int i = 0; i < NUM_PROCESS_QUEUE_LEVELS; i++) {
        os->queues[i] = List_create();
    }

    os->sendQueue = List_create();
    os->recvQueue = List_create();

    //initial process creation
    PCB init_process;
    init_process.priority = LOW; //This could be whatever
    init_process.status = RUNNING;
    os->INIT_PROCESS_PID = &init_process;
    os->running_process = &init_process;
    os->process_count = 1;

    List *init_queue = os->queues[init_process.priority];
    List_append(init_queue, &init_process);

    printf("Success: Init process created with PID %p\n", &init_process);
}

void create(OS *os, Priority priority) {

    if (os->process_count >= MAX_PROCESSES) {
        printf("Failure: Maximum number of processes reached.\n");
        return;
    }

    PCB new_process;
    new_process.priority = priority;
    new_process.status = READY;

    // Add the new process to the appropriate ready queue based on its priority
    List *ready_queue = os->queues[priority];
    List_append(ready_queue, &new_process);

    printf("Success: Process created with PID %p.\n", &new_process);
    os->process_count++;
}

// Function to fork a process (create a copy) and add it to the ready queue
void forkk(OS *os) {
    // Get the currently running process
    PCB* current_process = os->running_process;

    // Check if the current process is the init process
    if (current_process == os->INIT_PROCESS_PID) {
        printf("Failure: Forking the 'init' process is not allowed.\n");
        return;
    }

    // Check if the maximum number of processes has been reached
    if (os->process_count >= MAX_PROCESSES) {
        printf("Failure: Maximum number of processes reached.\n");
        return;
    }

    PCB new_process;
    new_process.priority = current_process->priority;
    new_process.status = READY;

    // Add the new process to the appropriate ready queue based on its priority
    List *ready_queue = os->queues[new_process->priority];
    List_append(ready_queue, new_process);
    os->process_count++;

    printf("Success: Process forked and added to the ready queue.\n");
}

// Function to kill the specified process and remove it from the system
void kill(OS *os, PCB* target_pid) {
    if (os->process_count == 1) {
        printf("shutting down...\n");
        exit(EXIT_SUCCESS);
    }
    if (target_pid == os->running_process) {
        quantum()
    }

    // Search for the process in all queues
    for (int i = 0; i < NUM_PROCESS_QUEUE_LEVELS; i++) {
        List *queue = os->queues[i];
        List_first(queue);
        while (List_curr(queue) != NULL) {
            PCB* process = (PCB*)List_curr(queue);
            if (process == target_pid) {
                // Found the process, remove it from the queue
                List_remove(queue);
                printf("Success: Process with PID %x killed and removed from the system.\n", &target_pid);
                os->process_count--;
                return;
            }
            List_next(queue);
        }
    }
    // Process not found
    printf("Failure: Process with PID %x not found.\n", &target_pid);
}

// Function to exit the currently running process
void exitOS(OS *os) {
    if (os->process_count == 1) {
        printf("shutting down...\n");
        exit(EXIT_SUCCESS);
    }

    // Find the currently running process
    PCB *current_process = os->running_process;

    // Check if the current process is the init process
    if (current_process == os->INIT_PROCESS_PID) {
        // If the current process is the init process, check if there are other active processes
        int active_processes = 0;
        for (int i = 0; i < NUM_PROCESS_QUEUE_LEVELS; i++) {
            active_processes += List_count(os->queues[i]);
        }
        // If there are other active processes, do not exit the init process
        if (active_processes > 1) {
            printf("Error: Cannot exit the init process as other processes are still active.\n");
            return;
        }
    }

    // Update the status of the current process to TERMINATED
    current_process->status = TERMINATED;

    printf("Success: Current process exited.\n");

    // Check if all processes are terminated
    int active_processes = 0;
    for (int i = 0; i < NUM_PROCESS_QUEUE_LEVELS; i++) {
        active_processes += List_count(os->queues[i]);
    }

    // If all processes are terminated (except the init process), exit the operating system
    if (active_processes == 0 && current_process != os->INIT_PROCESS_PID) {
        printf("All processes terminated. Exiting the operating system.\n");
        exit(EXIT_SUCCESS);
    } else {
        // Find the next process to run (assuming round-robin scheduling)
        for (int i = 0; i < NUM_PROCESS_QUEUE_LEVELS; i++) {
            List *queue = os->queues[i];
            if (List_count(queue) > 0) {
                os->running_process = (PCB *) List_curr(queue);
                printf("Success: Next process scheduled and running.\n");
                return; // Exit the loop after finding the first non-empty queue
            }
        }
    }
}

// Function to handle the quantum expiry (time quantum of the running process)
void quantum(OS *os, bool kill_process) {
    //kick off current process

    //logic for choosing the process based on round robin
    if (os->running_process == NULL) {
        printf("No process is currently running.\n");
        return;
    }

    // Find the priority of the currently running process
    Priority priority = os->running_process->priority;

    // Move the currently running process to the end of its priority queue
    if (kill_process == false) {
        if (List_count(queue) == 0) {
            printf("Error: The queue for the running process is empty.\n");
            return;
        }
    }

    List* queue = os->queues[priority];
    if (List_count(queue) == 0) {
        printf("Error: The queue for the running process is empty.\n");
        return;
    }

    List_first(queue);
    PCB* curr_process = (PCB*) List_curr(queue);
    if (curr_process != os->running_process) {
        printf("Error: Current process not found at the front of the queue.\n");
        return;
    }
    List_remove(queue);

    int result = List_append(queue, os->running_process);
    if (result != 0) {
        printf("Error: Failed to append the process back to the queue.\n");
        return;
    }

    PCB* next_process = NULL;
    for (int i = 0; i < NUM_PROCESS_QUEUE_LEVELS; i++) {
        List* higher_priority_queue = os->queues[i];
        List_first(higher_priority_queue);
        PCB* process = (PCB*) List_curr(higher_priority_queue);
        if (process != NULL && !process->Turn) {
            next_process = process;
            break;
        }
    }

    if (next_process == NULL) {
        for (int i = 0; i < NUM_PROCESS_QUEUE_LEVELS; i++) {
            List* curr_queue = os->queues[i];
            Node* process_node = curr_queue->head;
            while (process_node != NULL) {
                PCB* process = (PCB*) process_node->item;
                if (process != NULL) {
                    process->Turn = true;
                }
                process_node = process_node->next;
            }
        }
    } 
    else {
        os->running_process = next_process;
        next_process->Turn = true;
        printf("Time quantum of the running process expired.\n");
        printf("Process with PID %p now gets control of the CPU.\n", (void *)os->running_process);
    }
}

// Function to send a message to another process and block until a reply is received
void send(OS *os, PCB* target_pid, char *msg) {
    //find the target process
    PCB *target_process = NULL;
    for (int i = 0; i < NUM_PROCESS_QUEUE_LEVELS; i++) {
        List *queue = os->queues[i];
        List_first(queue);
        while (List_curr(queue) != NULL) {
            PCB *process = (PCB *) List_curr(queue);
            if (process == target_pid) {
                target_process = process;
                break;
            }
            List_next(queue);
        }
        if (target_process != NULL) {
            break;
        }
    }

    if (target_process == NULL) {
        printf("Failure: Process with PID %d not found.\n", target_pid);
        return;
    }

    // Block sender, store message, and wait for reply
    PCB *sender_process = os->running_process;
    sender_process->status = BLOCKED;

    strcpy(target_process->proc_message, msg);
    printf("Success: Message sent to process with PID %d: %s\n", target_pid, msg);

    printf("Waiting for reply...\n");
}

// Function to receive a message and block until one arrives
void receive(OS *os) {
    // Assume the currently running process is receiving the message
    PCB *receiver_process = os->running_process;

    // Check if there's a message waiting for the receiver
    if (strlen(receiver_process->proc_message) == 0) {
        printf("No message available for the currently running process. Blocking until one arrives...\n");
        //I am not sure how to block the process here but I beilive we have to use a conbination of semaphors
    }

    printf("Received message from process with PID %d: %s\n", receiver_process->sender_pid, receiver_process->proc_message);
    // Clear the message from the receiver process
    receiver_process->sender_pid = 0;
    receiver_process->proc_message[0] = '\0';
}

// Function to make a reply to a process and unblock the sender
void reply(OS *os, PCB* reply_pid, char *reply_msg) {
    // Find the process to reply to
    PCB *reply_process = NULL;
    for (int i = 0; i < NUM_PROCESS_QUEUE_LEVELS; i++) {
        List *queue = os->queues[i];
        List_first(queue);
        while (List_curr(queue) != NULL) {
            PCB *process = (PCB *) List_curr(queue);
            if (process == reply_pid) {
                reply_process = process;
                break;
            }
            List_next(queue);
        }
        if (reply_process != NULL) {
            break;
        }
    }

    if (reply_process == NULL) {
        printf("Failure: Process with PID %d not found.\n", reply_pid);
        return;
    }

    // Unblock the sender process
    PCB *sender_process = os->running_process;
    sender_process->status = READY;

    // Set the message in the reply process
    strcpy(reply_process->proc_message, reply_msg);

    printf("Success: Reply sent to process with PID %d.\n", reply_pid);
}

// Function to initialize a semaphore with the given ID and initial value
void semaphore(OS *os, int semaphore_id, int initial_value) {
    if (semaphore_id < 0 || semaphore_id >= MAX_SEMAPHORES) {
        printf("Failure: Invalid semaphore ID. It should be in the range [0, 4].\n");
        return;
    }

    Sem* sem = os->semaphores[semaphore_id];
    sem->exists = false;
    if (sem->exists == false) {
        sem->exists = true;
        sem->value = initial_value;        
        printf("Success: Semaphore %d initialized with initial value %d.\n", semaphore_id, initial_value);
    }
    else {
        printf("Faliure: semaphore already exists!\n");
    }
}

// Function to perform the P operation on a semaphore
void semaphore_P(OS *os, int semaphore_id) {
    // Check if semaphore_id is within valid range
    if (semaphore_id < 0 || semaphore_id >= MAX_SEMAPHORES || os->semaphores[semaphore_id]->exists == false){
        printf("Failure: Invalid semaphore ID. Please enter the value of an existing semaphore.\n");
        return;
    }

    Sem* sem = os->semaphores[semaphore_id];

    if (sem->value > 0) { //decrement the semaphore
        sem->value--;
        printf("Success: process not blocked.\n");
    }

    else if (sem == NULL) { //block the process
        PCB* current_process = os->running_process;
        if (current_process == os->INIT_PROCESS_PID) { //do not block
            printf("Faliure: init process cannot be blocked.\n");
            return;
        }
        current_process->status = BLOCKED;

        List* wait_queue = os->semaphore_wait_queues[semaphore_id];
        List_append(wait_queue, current_process);
        quantum(os); //this should remove the current process from the CPU
        printf("Success: process blocked on the semaphore.\n");
    }
}

// Function to perform the V operation on a semaphore
void semaphore_V(OS *os, int semaphore_id) {
    // Check if the semaphore ID is valid
    if (semaphore_id < 0 || semaphore_id >= MAX_SEMAPHORES || os->semaphores[semaphore_id]->exists == false){
        printf("Failure: Invalid semaphore ID. Please enter the value of an existing semaphore.\n");
        return;
    }

    // Increment the semaphore value
    os->semaphores[semaphore_id]->value--;

    // If there are processes waiting on this semaphore,
    // unblock the first waiting process and move it to the appropriate priority queue
    List* sem_queue = os->semaphore_wait_queues[semaphore_id];
    if (List_count(sem_queue) > 0) {
        //pop the front of the process queue
        List_first(sem_queue);
        PCB* process = List_remove(sem_queue);

        List* priority_queue = os->queues[process->priority];
        List_append(priority_queue, process);

        printf("Success: Process with PID %x readied.\n", process);
    }
    else {
        printf("Success: no process readied.\n");
    }
}

// Function to display complete state information of a process
void process_info(OS *os, PCB* pid) {
    // Find the process with the given PID
    PCB *target_process = NULL;
    for (int i = 0; i < NUM_PROCESS_QUEUE_LEVELS; i++) {
        List *queue = os->queues[i];
        List_first(queue);
        while (List_curr(queue) != NULL) {
            PCB *process = (PCB *) List_curr(queue);
            if (process == (PCB *) pid) {
                target_process = process;
                break;
            }
            List_next(queue);
        }
        if (target_process != NULL) {
            break;
        }
    }

    if (target_process == NULL) {
        printf("Failure: Process with PID %d not found.\n", &pid);
        return;
    }

    printf("Process Information for PID %d:\n", pid);
    printf("Priority: %d\n", target_process->priority);
    printf("Status: ");
    switch (target_process->status) {
        case READY:
            printf("Ready\n");
            break;
        case BLOCKED:
            printf("Blocked\n");
            break;
        case RUNNING:
            printf("Running\n");
            break;
        case TERMINATED:
            printf("Terminated\n");
            break;
        default:
            printf("Unknown\n");
    }
    printf("Message: %s\n", target_process->proc_message);
}

// Function to display all process queues and their contents
void total_info(OS *os) {
    printf("Total Information:\n");

    // Display information about processes in each queue
    for (int i = 0; i < NUM_PROCESS_QUEUE_LEVELS; i++) {
        List *queue = os->queues[i];
        printf("Queue %d:\n", i);
        printf("Processes:\n");
        List_first(queue);
        while (List_curr(queue) != NULL) {
            PCB *process = (PCB *) List_curr(queue);
            process_info(&os, process);
            List_next(queue);
        }
        printf("\n");
    }

    // Display information about semaphores
    printf("Semaphores:\n");
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        Sem *semaphore = os->semaphores[i];
        if (semaphore != NULL) {
            printf("Semaphore ID: %d, Initial Value: %d\n", i, semaphore->value);
            // Display information about processes waiting on this semaphore
            List *wait_queue = os->semaphore_wait_queues[i];
            printf("Processes waiting on this semaphore:\n");
            List_first(wait_queue);
            while (List_curr(wait_queue) != NULL) {
                PCB *process = (PCB *) List_curr(wait_queue);
                printf("Process: %p\n", &process);
                List_next(wait_queue);
            }
            printf("\n");
        }
    }

    // Display information about the send queue
    printf("Send Queue:\n");
    List_first(os->sendQueue);
    while (List_curr(os->sendQueue) != NULL) {
        PCB *process = (PCB *) List_curr(os->sendQueue);
        printf("Process: %p\n", &process);
        List_next(os->sendQueue);
    }
    printf("\n");

    // Display information about the receive queue
    printf("Receive Queue:\n");
    List_first(os->recvQueue);
    while (List_curr(os->recvQueue) != NULL) {
        PCB *process = (PCB *) List_curr(os->recvQueue);
        printf("Process: %p\n", &process);
        List_next(os->recvQueue);
    }
    printf("\n");
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
                while (scanf("%d", &priority) != 1 || priority < 0 || priority > 2) {
                    printf("Invalid input. Priority must be 0, 1, or 2. Please try again: ");
                    while (getchar() != '\n'); // Clear input buffer
                }
                create(&os, priority);
                break;
            }
            case 'F':
                printf("Forking the currently running process...\n");
                forkk(&os);
                break;
            case 'K': {
                PCB* pid;
                printf("Enter PID of the process to be killed: ");
                while (scanf("%x", &pid) != 1) {
                    printf("Invalid input. Please enter a valid integer PID: ");
                    while (getchar() != '\n'); // Clear input buffer
                }
                kill(&os, pid);
                break;
            }
            case 'E':
                printf("Exiting the currently running process...\n");
                exitOS(&os);
                break;
            case 'Q':
                printf("Time quantum of the running process expired...\n");
                quantum(&os, false);
                break;
            case 'S': {
                PCB* target_pid;
                char message[MAX_MESSAGE_LENGTH];
                printf("Enter PID of the process to send message to: ");
                while (scanf("%d", &target_pid) != 1) {
                    printf("Invalid input. Please enter a valid integer PID: ");
                    while (getchar() != '\n'); // Clear input buffer
                }
                printf("Enter message (max 40 characters): ");
                scanf("%s", message);
                send(&os, target_pid, message);
                break;
            }
            case 'R':
                printf("Waiting to receive a message...\n");
                receive(&os);
                break;
            case 'Y': {
                PCB* reply_pid;
                char reply_message[MAX_MESSAGE_LENGTH];
                printf("Enter PID of the process to make the reply to: ");
                while (scanf("%x", &reply_pid) != 1) {
                    printf("Invalid input. Please enter a valid integer PID: ");
                    while (getchar() != '\n'); // Clear input buffer
                }
                printf("Enter reply message (max 40 characters): ");
                scanf("%s", reply_message);
                reply(&os, reply_pid, reply_message);
                break;
            }
            case 'N': {
                int semaphore_id, initial_value;
                printf("Enter semaphore ID (0-4): ");
                while (scanf("%d", &semaphore_id) != 1 || semaphore_id < 0 || semaphore_id > 4) {
                    printf("Invalid input. Semaphore ID must be between 0 and 4. Please try again: ");
                    while (getchar() != '\n'); // Clear input buffer
                }
                printf("Enter initial value: ");
                while (scanf("%d", &initial_value) != 1) {
                    printf("Invalid input. Please enter a valid integer initial value: ");
                    while (getchar() != '\n'); // Clear input buffer
                }
                semaphore(&os, semaphore_id, initial_value);
                break;
            }
            case 'P': {
                int semaphore_id;
                printf("Enter semaphore ID (0-4) for P operation: ");
                while (scanf("%d", &semaphore_id) != 1 || semaphore_id < 0 || semaphore_id > 4) {
                    printf("Invalid input. Semaphore ID must be between 0 and 4. Please try again: ");
                    while (getchar() != '\n'); // Clear input buffer
                }
                semaphore_P(&os, semaphore_id);
                break;
            }
            case 'V': {
                int semaphore_id;
                printf("Enter semaphore ID (0-4) for V operation: ");
                while (scanf("%d", &semaphore_id) != 1 || semaphore_id < 0 || semaphore_id > 4) {
                    printf("Invalid input. Semaphore ID must be between 0 and 4. Please try again: ");
                    while (getchar() != '\n'); // Clear input buffer
                }
                semaphore_V(&os, semaphore_id);
                break;
            }
            case 'I': {
                PCB* pid;
                printf("Enter PID of the process for which information is to be returned: ");
                while (scanf("%x", &pid) != 1) {
                    printf("Invalid input. Please enter a valid integer PID: ");
                    while (getchar() != '\n'); // Clear input buffer
                }
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

    } while (os.process_count > 0);

    return 0;
}