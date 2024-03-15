#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"


#define MAX_PROCESSES 100
#define MAX_MESSAGE_LENGTH 40
#define MAX_SEMAPHORES 10

typedef enum { HIGH, NORMAL, LOW } Priority;
typedef enum { READY, BLOCKED, RUNNING, TERMINATED } Status;

typedef struct {
    int pid;
    Priority priority;
    Status status;
    char proc_message[MAX_MESSAGE_LENGTH];
    int sender_pid;
} PCB;

typedef struct {
    PCB processes[MAX_PROCESSES];
    int process_count;
    List* queues[3];  // Low, Medium, High priority queues
    PCB *running_process;  // Pointer to the currently running process
    int semaphores[MAX_SEMAPHORES];
    List *semaphore_wait_queues[5];  // Wait queues for semaphores
} OS;

List lists[LIST_MAX_NUM_HEADS];
Node nodes[LIST_MAX_NUM_NODES];

void init(OS *os) {
    //Initialize the operating system
    os->process_count = 0;

    for (int i = 0; i < 3; i++) {
        os->queues[i] = List_create();
        if (!os->queues[i]) {
            printf("Failure: Unable to create priority queue %d.\n", i);
            exit(EXIT_FAILURE);
        }
    }

    //initial process creation
    PCB init_process;
    init_process.pid = (intptr_t)&init_process;
    init_process.priority = HIGH; //This could be whatever
    init_process.status = READY;

    List *init_queue = os->queues[init_process.priority];
    List_append(init_queue, &init_process);

    os->processes[os->process_count++] = init_process;

    printf("Success: Init process created with PID %d\n", (void *)(intptr_t)init_process.pid);
}

int create(OS *os, Priority priority) {

    if (os->process_count >= MAX_PROCESSES) {
        printf("Failure: Maximum number of processes reached.\n");
        return -1; 
    }

    PCB new_process;
    new_process.pid = (intptr_t)&new_process; //using intptr_t for pointer to integer
    new_process.priority = priority;
    new_process.status = READY;

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

    printf("Sucess: Process created with PID  %d\n", (void*)(intptr_t)new_process.pid);
    return new_process.pid; //success
}

// Function to fork a process (create a copy) and add it to the ready queue
void fork(OS *os) {
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
    if (current_process->pid == (intptr_t)&current_process) {
        printf("Failure: Forking the 'init' process is not allowed.\n");
        return;
    }

    //Copy the process to its queue
    PCB new_process;
    new_process.pid = (intptr_t)&new_process;
    new_process.priority = current_process->priority;
    new_process.status = READY;

    List *ready_queue = os->queues[current_process->priority];
    List_append(ready_queue, &new_process);

    os->processes[os->process_count++] = new_process;

    printf("Scuess: Process forked with PID  %d\n", (void *)(intptr_t)new_process.pid);
}

// Function to kill the specified process and remove it from the system
void kill(OS *os, int pid) {
    // Search for the process with the given PID
    PCB *process_to_kill = NULL;
    for (int i = 0; i < os->process_count; i++) {
        if (os->processes[i].pid == pid) {
            process_to_kill = &(os->processes[i]);
            break;
        }
    }

    if (process_to_kill == NULL) {
        printf("Failure: Process with PID %d not found.\n", pid);
        return;
    }

    // Remove the process from the queues and mark it as terminated
    for (int i = 0; i < 3; i++) {
        List *queue = os->queues[i];
        Node *current_node = queue->head;
        while (current_node != NULL) {
            PCB *current_process = (PCB *)current_node->item;
            if (current_process == process_to_kill) {
                List_remove_at(queue, current_node); // Remove the process from the queue
                printf("Success: Process with PID %d has been killed.\n", pid);
                process_to_kill->status = TERMINATED; // Mark the process as terminated
                return;
            }
            current_node = current_node->next;
        }
    }
    printf("Failure: Process with PID %d not found in any queue.\n", pid);
}

// Function to exit the currently running process
void exit(OS *os) {
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

    printf("Process with PID %d terminated.\n", current_process->pid);

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
                printf("Process with PID %d now gets control of the CPU.\n", next_process->pid);
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
    // Implement Quantum expiry logic here
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
            if (process->pid == target_pid) {
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
    PCB *sender_process = get_running_process(os);
    sender_process->status = BLOCKED;

    strcpy(target_process->proc_message, msg);
    printf("Success: Message sent to process with PID %d: %s\n", target_pid, msg);

    printf("Waiting for reply...\n");
}

// Function to receive a message and block until one arrives
void receive(OS *os) {
    // Assume the currently running process is receiving the message
    PCB *receiver_process = get_running_process(os);

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
        if (os->processes[i].pid == reply_pid) {
            reply_process = &(os->processes[i]);
            break;
        }
    }

    if (reply_process == NULL) {
        printf("Failure: Process with PID %d not found.\n", reply_pid);
        return;
    }

    // Unblock the sender process
    PCB *sender_process = get_running_process(os);
    sender_process->status = READY;

    // Set the message in the reply process
    strcpy(reply_process->proc_message, reply_msg);

    printf("Success: Reply sent to process with PID %d.\n", reply_pid);
}

// Function to initialize a semaphore with the given ID and initial value
void semaphore(OS *os, int semaphore_id, int initial_value) {
    if (semaphore_id < 0 || semaphore_id >= 5) {
        printf("Failure: Invalid semaphore ID. It should be in the range [0, 4].\n");
        return;
    }

    // Initialize the semaphore with the given initial value
    os->semaphores[semaphore_id] = initial_value;

    printf("Success: Semaphore %d initialized with initial value %d.\n", semaphore_id, initial_value);
}

// Function to perform the P operation on a semaphore
void semaphore_P(OS *os, int semaphore_id) {
    // Check if semaphore_id is within valid range
    if (semaphore_id < 0 || semaphore_id >= 5) {
        printf("Failure: Invalid semaphore ID. Please enter a value between 0 and 4.\n");
        return;
    }

    os->semaphores[semaphore_id]--;

    // If the semaphore value becomes negative, block the process
    if (os->semaphores[semaphore_id] < 0) {

        PCB *current_process = get_running_process(os);
        current_process->status = BLOCKED;

        List *wait_queue = os->semaphore_wait_queues[semaphore_id];
        List_append(wait_queue, current_process);
    }
}

// Function to perform the V operation on a semaphore
void semaphore_V(OS *os, int semaphore_id) {
   // Check if the semaphore ID is valid
    if (semaphore_id < 0 || semaphore_id >= 5) {
        printf("Invalid semaphore ID. Semaphore IDs range from 0 to 4.\n");
        return;
    }

    // Increment the semaphore value
    os->semaphores[semaphore_id]++;

    // If there are processes waiting on this semaphore and the semaphore value is non-negative,
    // unblock the first waiting process and move it to the appropriate priority queue
    if (os->semaphores[semaphore_id] <= 0) {
        List *wait_queue = os->semaphore_wait_queues[semaphore_id];
        if (!wait_queue) {
            printf("Failure: Semaphore wait queue not initialized for semaphore ID %d.\n", semaphore_id);
            return;
        }

        // Remove the first process from the wait queue
        if (List_count(wait_queue) > 0) {
            PCB *waiting_process = (PCB *)List_remove_at(wait_queue, 0);
            waiting_process->status = READY;

            // Add the waiting process to the appropriate priority queue based on its priority
            List *priority_queue = os->queues[waiting_process->priority];
            if (!priority_queue) {
                printf("Failure: Priority queue not initialized for priority %d.\n", waiting_process->priority);
                return;
            }
            List_append(priority_queue, waiting_process);

            printf("Success: Process with PID %d unblocked.\n", waiting_process->pid);
        }
    }
}

// Function to display complete state information of a process
void process_info(OS *os, int pid) {
    // Find the process with the given PID
    PCB *process = NULL;
    for (int i = 0; i < os->process_count; i++) {
        if (os->processes[i].pid == pid) {
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
    printf("PID: %d\n", process->pid);
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
            fork(&os);
            break;
        case 'K': {
            int pid;
            printf("Enter PID of the process to be killed: ");
            while (scanf("%d", &pid) != 1) {
                printf("Invalid input. Please enter a valid integer PID: ");
                while (getchar() != '\n'); // Clear input buffer
            }
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
            recieve(&os);
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
            semaphor(&os, semaphore_id, initial_value);
            break;
        }
        case 'P': {
            int semaphore_id;
            printf("Enter semaphore ID (0-4) for P operation: ");
            while (scanf("%d", &semaphore_id) != 1 || semaphore_id < 0 || semaphore_id > 4) {
                printf("Invalid input. Semaphore ID must be between 0 and 4. Please try again: ");
                while (getchar() != '\n'); // Clear input buffer
            }
            semaphor_P(&os, semaphore_id);
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