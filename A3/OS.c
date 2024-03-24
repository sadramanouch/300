#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "list.c"

#define MAX_PROCESSES 10
#define MAX_MESSAGE_LENGTH 40
#define MAX_SEMAPHORES 5
#define NUM_PROCESS_QUEUE_LEVELS 3

typedef enum { HIGH, NORMAL, LOW } Priority;
typedef enum { READY, BLOCKED, RUNNING, TERMINATED } Status;

typedef struct {
    bool exists = false;
    int value;
} Sem;

typedef struct {
    Priority priority;
    Status status;
    char proc_message[MAX_MESSAGE_LENGTH];
    PCB* sender_pid;
} PCB;

typedef struct {
    //PCB processes[MAX_PROCESSES];
    int process_count;
    PCB* INIT_PROCESS_PID;
    List* queues[NUM_PROCESS_QUEUE_LEVELS];  // Low, Medium, High priority queues
    List* sendQueue;  // processes that have sent and are waiting for a reply
    List* recvQueue;  // processes that have called receive and are waiting
    PCB* running_process;  // Pointer to the currently running process
    Sem* semaphores[MAX_SEMAPHORES]; // holds semaphores
    List* semaphore_wait_queues[MAX_SEMAPHORES];  // Wait queues for semaphores
} OS;

void init(OS *os) {
    //Initialize the operating system

    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        Sem* newSem;
        semaphores[i] = newSem;
        semaphore_wait_queues[i] = List_create();
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

int create(OS *os, Priority priority) {

    if (os->process_count >= MAX_PROCESSES) {
        printf("Failure: Maximum number of processes reached.\n");
        return -1; 
    }

    PCB new_process;
    new_process.pid = (int*)(intptr_t)&new_process; //using intptr_t for pointer to integer
    new_process.priority = priority;
    new_process.status = RUNNING;

    List* newQueue = os->queues[priority];
    if (!newQueue) {
        printf("Failure: Unable to access priority queue %d.\n", priority);
        return -1; 
    }
    List_append(newQueue, &new_process);

    os->processes[os->process_count++] = new_process;

    //I was not able to implement the quantum because I was not sure how to tackle it but here, the running process pointer 
    //should be updates everytime a new process gets its turn and runs wth the quantum implementation
    if (os->running_process == NULL) {
        os->running_process = &new_process;
    }

    printf("Sucess: Process created with PID %p\n", new_process.pid);
    return *(new_process.pid); //success
}

// Function to fork a process (create a copy) and add it to the ready queue
void forkk(OS *os) {
    // Get the currently running process
    PCB *current_process = NULL;
    for (int i = 0; i < os->process_count; i++) {
        if (os->processes[i].status == RUNNING) {
            current_process = &os->processes[i];
            break;
        }
    }

    // If no running process is found, return
    if (current_process == NULL) {
        printf("Failure: No running process to fork.\n");
        return;
    }

    // Check if the current process is the init process
    if (*current_process->pid == (intptr_t)current_process) {
        printf("Failure: Forking the 'init' process is not allowed.\n");
        return;
    }

    //Copy the process to its queue
    PCB new_process;
    new_process.pid = (int*)(intptr_t)&new_process;
    new_process.priority = current_process->priority;
    new_process.status = READY;

    List *ready_queue = os->queues[current_process->priority];
    List_append(ready_queue, &new_process);

    os->processes[os->process_count++] = new_process;

    printf("Scuess: Process forked with PID  %p\n", new_process.pid);
}

// Function to kill the specified process and remove it from the system
void kill(OS *os, PCB* target_pid) {
    // Search for the process in all queues
    if (os->process_count <= 1) {
        printf("Shutting down...\n");
        exit(EXIT_SUCCESS);
    }
    for (int i = 0; i < 3; ++i) {
        List *queue = os->queues[i];
        List_first(queue);
        while (List_curr(queue) != NULL) {
            PCB *pcb = (PCB *) List_curr(queue);
            if (*(pcb->pid) == target_pid) {
                // Found the process, remove it from the queue
                List_remove(queue);
                printf("Process with PID %d killed.\n", target_pid);
                return;
            }
            List_next(queue);
        }
    }
    // Process not found
    printf("Failure: Process with PID %d not found in any queue.\n", target_pid);
}

// Function to exit the currently running process
void exitOS(OS *os) {
    // Find the currently running process
    PCB *current_process = NULL;
    for (int i = 0; i < os->process_count; i++) {
        if (os->processes[i].status == RUNNING) {
            current_process = &os->processes[i];
            break;
        }
    }

    // If no running process is found, return
    if (current_process == NULL) {
        printf("Failure: No running process to exit.\n");
        return;
    }

    // Update the status of the current process to TERMINATED
    current_process->status = TERMINATED;

    printf("Process with PID %p terminated.\n", current_process->pid);

    // Check if all processes, including init, are terminated
    int active_processes = 0;
    for (int i = 0; i < os->process_count; i++) {
        if (os->processes[i].status != TERMINATED) {
            active_processes++;
        }
    }

    // If all processes are terminated, exit the operating system
    if (active_processes == 0) {
        printf("All processes terminated. Exiting the operating system.\n");
        exit(EXIT_SUCCESS);
    } 
    else {
    // Find the next process to run (assuming round-robin scheduling)
        for (int i = 0; i < 3; i++) {
            List *queue = os->queues[i];
            if (List_count(queue) > 0) {
                PCB *next_process = ((Node *)List_first(queue))->item;
                printf("Process with PID %p now gets control of the CPU.\n", next_process->pid);
                break; // Exit the loop after finding the first non-empty queue
            }
        }
    }

    if (os->process_count == 1) {
        // Clear the running_process pointer
        os->running_process = NULL;
    }
}

// Function to handle the quantum expiry (time quantum of the running process)
void quantum(OS *os) {
    //kick off current process

    //logic for choosing the process based on round robin
}

// Function to send a message to another process and block until a reply is received
void send(OS *os, int target_pid, char *msg) {
    //find the target process
    PCB *target_process = NULL;
    for (int i = 0; i < 3; i++) {
        List *queue = os->queues[i];
        Node *current_node = queue->head;
        while (current_node != NULL) {
            PCB *process = (PCB *)current_node->item;
            if (*process->pid == target_pid) {
                target_process = process;
                break;
            }
            current_node = current_node->next;
        }
        if (target_process != NULL) {
            break;
        }
    }

    if (target_process == NULL) {
        printf("Failure: Process with PID %d not found.\n", target_pid);
        return;
    }

    //block sender, store message and wait for reply
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
    receiver_process->sender_pid = -1;
    receiver_process->proc_message[0] = '\0';
}

// Function to make a reply to a process and unblock the sender
void reply(OS *os, int reply_pid, char *reply_msg) {
    // Find the process to reply to
    PCB *reply_process = NULL;
    for (int i = 0; i < os->process_count; i++) {
        if (*os->processes[i].pid == reply_pid) {
            reply_process = &(os->processes[i]);
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
        printf("Success: process not blocked.\n")
    }

    else if (sem == 0) { //block the process
        PCB* current_process = os->running_process;
        if (current_process == os->INIT_PROCESS_PID) { //do not block
            printf("Faliure: init process cannot be blocked.\n")
            return;
        }
        current_process->status = BLOCKED;

        List* wait_queue = os->semaphore_wait_queues[semaphore_id];
        List_append(wait_queue, current_process);
        quantum(os); //this should remove the current process from the CPU
        printf("Success: process blocked on the semaphore.\n")
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
    os->semaphores[semaphore_id]++;

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
        printf("Success: no process readied.\n")
    }
}

// Function to display complete state information of a process
void process_info(OS *os, int pid) {
    // Find the process with the given PID
    PCB *process = NULL;
    for (int i = 0; i < os->process_count; i++) {
        if (*os->processes[i].pid == pid) {
            process = &(os->processes[i]);
            break;
        }
    }

    if (process == NULL) {
        printf("Process with PID %d not found.\n", pid);
        return;
    }

    // Display process information
    printf("Process Information:\n");
    printf("PID: %p\n", process->pid);
    printf("Priority: %d\n", process->priority);
    printf("Status: ");
    switch (process->status) {
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
    printf("Message: %s\n", process->proc_message);
}

// Function to display all process queues and their contents
void total_info(OS *os) {
    //I am not sure if the Queue implementation is correct so I dod not fill out this function
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
            quantum(&os);
            break;
        case 'S': {
            int target_pid;
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
            int reply_pid;
            char reply_message[MAX_MESSAGE_LENGTH];
            printf("Enter PID of the process to make the reply to: ");
            while (scanf("%d", &reply_pid) != 1) {
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
            int pid;
            printf("Enter PID of the process for which information is to be returned: ");
            while (scanf("%d", &pid) != 1) {
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