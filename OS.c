#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "OS.h"

void cleanup(OS* os) {
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        free(os->semaphores[i]);
    }

    free(os->INIT_PROCESS_PID);
}

int numReadyProcesses(OS* os) {
    int count = 0;
    for (int i = 0; i < NUM_PROCESS_QUEUE_LEVELS; i++) {
        List* queue = os->queues[i];
        count += List_count(queue);
    }
    return count;
}

void init(OS *os) {
    //Initialize the operating system

    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        Sem* newSem = (Sem*) malloc(sizeof(Sem));
        os->semaphores[i] = newSem;
        os->semaphore_wait_queues[i] = List_create();
    }

    for (int i = 0; i < NUM_PROCESS_QUEUE_LEVELS; i++) {
        os->queues[i] = List_create();
    }

    os->sendQueue = List_create();
    os->recvQueue = List_create();

    //initial process creation
    PCB* init_process = (PCB*) malloc(sizeof(PCB));
    init_process->priority = LOW; //This could be whatever
    init_process->status = RUNNING;
    init_process->Turn = true;
    init_process->sender_pid = NULL;
    os->INIT_PROCESS_PID = init_process;
    os->running_process = init_process;
    os->process_count = 1;
    os->INIT_PROCESS_PID->Turn = false;
    List *init_queue = os->queues[init_process->priority];
    List_append(init_queue, init_process);

    printf("Success: Init process created with PID %p\n", init_process);
}

void create(OS *os, Priority priority) {

    if (os->process_count >= MAX_PROCESSES) {
        printf("Failure: Maximum number of processes reached.\n");
        return;
    }

    PCB* new_process = (PCB*) malloc(sizeof(PCB));
    new_process->priority = priority;
    new_process->status = READY;
    new_process->Turn = false;
    new_process->sender_pid = NULL;

    // Add the new process to the appropriate ready queue based on its priority
    List *ready_queue = os->queues[priority];
    List_append(ready_queue, new_process);

    printf("Success: Process created with PID %p.\n", new_process);
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

    PCB* new_process = (PCB*) malloc(sizeof(PCB));
    new_process->priority = current_process->priority;
    new_process->status = READY;
    new_process->Turn = false;
    new_process->sender_pid = NULL;

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
        cleanup(os);
        exit(EXIT_SUCCESS);
    }
    if (target_pid == os->running_process) {
        quantum(os, false, true);
        return;
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
                free(process);
                printf("Success: Process with PID %p killed and removed from the system.\n", target_pid);
                os->process_count--;
                return;
            }
            List_next(queue);
        }
    }
    //semaphore queues
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        List* queue = os->semaphore_wait_queues[i];
        List_first(queue);
        while (List_curr(queue) != NULL) {
            PCB* process = (PCB*)List_curr(queue);
            if (process == target_pid) {
                // Found the process, remove it from the queue
                List_remove(queue);
                free(process);
                printf("Success: Process with PID %p killed and removed from the system.\n", target_pid);
                os->process_count--;
                return;
            }
            List_next(queue);
        }
    }
    //send queue
    List* queue = os->sendQueue;
    List_first(queue);
    while (List_curr(queue) != NULL) {
        PCB* process = (PCB*)List_curr(queue);
        if (process == target_pid) {
            // Found the process, remove it from the queue
            List_remove(queue);
            free(process);
            printf("Success: Process with PID %p killed and removed from the system.\n", target_pid);
            os->process_count--;
            return;
        }
        List_next(queue);
    }
    //recv queue
    queue = os->recvQueue;
    List_first(queue);
    while (List_curr(queue) != NULL) {
        PCB* process = (PCB*)List_curr(queue);
        if (process == target_pid) {
            // Found the process, remove it from the queue
            List_remove(queue);
            free(process);
            printf("Success: Process with PID %p killed and removed from the system.\n", target_pid);
            os->process_count--;
            return;
        }
        List_next(queue);
    }
    // Process not found
    printf("Failure: Process with PID %p not found.\n", target_pid);
}

// Function to exit the currently running process
void exitOS(OS *os) {
    if (os->process_count == 1) {
        printf("shutting down...\n");
        cleanup(os);
        exit(EXIT_SUCCESS);
    }
    
    quantum(os, false, true);
}

// Function to handle the quantum expiry (time quantum of the running process)
void quantum(OS *os, Bool que, Bool kill_process) {
    //kick off the running process
    PCB* process = os->running_process;
    process->Turn = true;

    if(que){ // ready queue the process
    	List_append(os->queues[process->priority], process);
    	process->status = READY;
    }
    else if(kill_process){
    	free(process);
    }
    else{	//process is in a blocked queue
    	process->status = BLOCKED;
    }

    //choose a new process to run
    
    if(numReadyProcesses(os) == 1){
    	os->running_process = os->INIT_PROCESS_PID;
    	os->running_process->status = RUNNING;
    	return;
    }

    PCB* next_process = NULL;

    while (!next_process){

        for (int i = 0; i < NUM_PROCESS_QUEUE_LEVELS; i++) {
            List* queue = os->queues[i];
    	    List_first(queue);

    	    while(List_curr(queue)){
    		    PCB* new_process = List_curr(queue);

    		    if (new_process && new_process->Turn == false) {
    	    		List_remove(queue);
                	next_process = new_process;
    	    		os->running_process = next_process;
    	    		next_process->status = RUNNING;
    	    		next_process->Turn = true;
    			    printf("Success: process with PID %p is on the CPU.\n", next_process);
                    
                    if (next_process->sender_pid && next_process->proc_message[0] != 0) { // process was waiting on a receive and received
                        printf("Received message from %p: ", next_process->sender_pid);
                        puts(next_process->proc_message);
                        //clear the message
                        memset(next_process->proc_message, 0, MAX_MESSAGE_LENGTH);
                    }

                    return;
                }
    	        List_next(queue);
    	    }
            
        }
    
    	for(int i = 0; i< NUM_PROCESS_QUEUE_LEVELS; i++){
    		List* queue = os->queues[i];
    		List_first(queue);
    		while(List_curr(queue)){
    			PCB* curr_process = (PCB*)List_curr(queue);
                curr_process->Turn = false;
    			List_next(queue);
    		}
    	}
    	os->INIT_PROCESS_PID->Turn = true;
    }
    
    printf("ERROR: NO PROCESS ON CPU\n");
    exit(EXIT_FAILURE);
    return;

}

// Function to send a message to another process and block until a reply is received
void send(OS* os, PCB* target_pid, char* msg) {
    //init process cannot send
    if (os->running_process == os->INIT_PROCESS_PID) {
        printf("Faliure: cannot send from init process.\n");
        return;
    }

    //make sure the target process exists
    Bool exists = false;
    //check ready queues
    for (int i = 0; i < NUM_PROCESS_QUEUE_LEVELS; i++) {
        List* queue = os->queues[i];
        List_first(queue);
        while (List_curr(queue) != NULL) {

            PCB* process = (PCB*)List_curr(queue);
            if (process == target_pid) {
                exists = true;
                break;
            }

            List_next(queue);
        }
    }
    //check semaphore wait queues
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        List* queue = os->semaphore_wait_queues[i];
        List_first(queue);
        while (List_curr(queue) != NULL) {

            PCB* process = (PCB*)List_curr(queue);
            if (process == target_pid) {
                exists = true;
                break;
            }

            List_next(queue);
        }
    }
    //check send queue
    List* queue = os->sendQueue;
    List_first(queue);
    while (List_curr(queue) != NULL) {

        PCB* process = (PCB*)List_curr(queue);
        if (process == target_pid) {
            exists = true;
            break;
        }

        List_next(queue);
    }
    //check recv queue
    queue = os->recvQueue;
    List_first(queue);
    while (List_curr(queue) != NULL) {

        PCB* process = (PCB*)List_curr(queue);
        if (process == target_pid) {
            exists = true;
            break;
        }

        List_next(queue);
    }
    if (exists == false) {
        printf("Faliure: target process does not exist.\n");
        return;
    }

    // Block sender
    PCB* sender_process = os->running_process;
    sender_process->status = BLOCKED;
    List_append(os->sendQueue, sender_process);
    quantum(os, false, false);

    //clear the proc_message
    memset(target_pid->proc_message, 0, MAX_MESSAGE_LENGTH);

    //send msg
    strcpy(target_pid->proc_message, msg);
    target_pid->sender_pid = sender_process;
    printf("Success: Message sent to process with PID %p:\n", target_pid);
    puts(msg);
    printf("Waiting for reply...\n");

    //if receiver is blocked in the recvQueue, unblock and put it in a ready queue
    if (target_pid->status == BLOCKED) {
        List* queue = os->recvQueue;
        List_first(queue);
        while (List_curr(queue) != NULL) {

            PCB* process = (PCB*)List_curr(queue);
            if (process == target_pid) {
                List_remove(queue);
                process->status = READY;
                process->Turn = false;
                List_append(os->queues[process->priority], process);
                break;
            }

            List_next(queue);
        }
    }
}

// Function to receive a message and block until one arrives
void receive(OS *os) {

    PCB* receiver_process = os->running_process;

    // Check if there's a message waiting for the receiver
    if (receiver_process->sender_pid && receiver_process->proc_message[0] != 0) {
        printf("Received message from %p: ", receiver_process->sender_pid);
        puts(receiver_process->proc_message);
        //clear the message
        memset(receiver_process->proc_message, 0, MAX_MESSAGE_LENGTH);
    }
    else { // Block and wait for a message
        printf("No one has sent you a message, blocking process and waiting for a message.\n");
        receiver_process->status = BLOCKED;
        List_append(os->recvQueue, receiver_process);
        quantum(os, false, false);
    }

}

// Function to make a reply to a process and unblock the sender
void reply(OS* os, char* reply_msg) {

    PCB* reply_process = os->running_process;
    PCB* sender = reply_process->sender_pid;
    
    if (!sender) {
        printf("Faliure: no one sent you a message, therefore you cannot reply.\n");
        return;
    }

    // reply to the sender
    memset(sender->proc_message, 0, MAX_MESSAGE_LENGTH);
    strcpy(sender->proc_message, reply_msg);
    sender->sender_pid = reply_process;

    //unblock the sender, add the sender to a ready queue
    List* queue = os->sendQueue;
    List_first(queue);
    while (List_curr(queue) != NULL) {

        PCB* process = (PCB*)List_curr(queue);
        if (process == sender) {
            List_remove(queue);
            process->status = READY;
            process->Turn = false;
            List_append(os->queues[process->priority], process);
            break;
        }

        List_next(queue);
    }

    printf("Success: Reply sent to process with PID %p.\n", sender);
    reply_process->sender_pid = NULL;
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
        quantum(os, false, false); //this should remove the current process from the CPU
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
    os->semaphores[semaphore_id]->value++;

    // If there are processes waiting on this semaphore,
    // unblock the first waiting process and move it to the appropriate priority queue
    List* sem_queue = os->semaphore_wait_queues[semaphore_id];
    if (List_count(sem_queue) > 0) {
        //pop the front of the process queue
        List_first(sem_queue);
        PCB* process = List_remove(sem_queue);

        List* priority_queue = os->queues[process->priority];
        List_append(priority_queue, process);

        printf("Success: Process with PID %p readied.\n", process);
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
                printf("The process is in a ready queue.\n");
                break;
            }
            List_next(queue);
        }
        if (target_process != NULL) {
            break;
        }
    }

    //semaphore queues
    for (int i = 0; i < MAX_SEMAPHORES; i++){
        List *queue = os->queues[i];
        List_first(queue);
        while (List_curr(queue) != NULL) {
            PCB *process = (PCB *) List_curr(queue);
            if (process == (PCB *) pid) {
                target_process = process;
                printf("The process is blocked on semaphore %d.\n", i);
                break;
            }
            List_next(queue);
        }
        if (target_process != NULL) {
            break;
        }
    }

    //send queue
    List* queue = os->sendQueue;
    List_first(queue);
    while (List_curr(queue) != NULL) {
        PCB *process = (PCB *) List_curr(queue);
        if (process == (PCB *) pid) {
            target_process = process;
            printf("The process is blocked on a send operation.\n");
            break;
        }
        List_next(queue);
    }

    //recv queue
    queue = os->recvQueue;
    List_first(queue);
    while (List_curr(queue) != NULL) {
        PCB *process = (PCB *) List_curr(queue);
        if (process == (PCB *) pid) {
            target_process = process;
            printf("The process is blocked on a received operation.\n");
            break;
        }
        List_next(queue);
    }

    if (pid == os->INIT_PROCESS_PID){
        target_process = pid;
    }

    if (target_process == NULL) {
        printf("Failure: Process with PID %p not found.\n", &pid);
        return;
    }

    printf("Process Information for PID %p:\n", pid);
    
    printf("Priority: ");
    switch(target_process->priority){
        case HIGH:
            printf("HIGH\n");
            break;
        case NORMAL:
            printf("NORMAL\n");
            break;
        case LOW:
            printf("LOW\n");
            break;
        default:
            printf("Unknown\n");
    }
    
    printf("Status: ");
    switch (target_process->status) {
        case RUNNING:
            printf("Running\n");
            break;
        case TERMINATED:
            printf("Terminated\n");
            break;
        case READY:
            printf("Ready\n");
            break;
        case BLOCKED:
            printf("Blocked\n");
            break;
        default:
            printf("Unknown\n");
    }

}

// Function to display all process queues and their contents
void total_info(OS *os) {
    printf("OS PID's:\n");

    // Display information about processes in each queue
    for (int i = 0; i < NUM_PROCESS_QUEUE_LEVELS; i++) {
        List *queue = os->queues[i];
        printf("Queue %d:\n", i);
        printf("Processes:\n");
        List_first(queue);
        while (List_curr(queue) != NULL) {
            PCB *process = (PCB *) List_curr(queue);
            printf("-%p ,", (void *)process);
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
                printf("-%p ,", (void *)process);
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
        printf("-%p ,", (void *)process);
        List_next(os->sendQueue);
    }
    printf("\n");

    // Display information about the receive queue
    printf("Receive Queue:\n");
    List_first(os->recvQueue);
    while (List_curr(os->recvQueue) != NULL) {
        PCB *process = (PCB *) List_curr(os->recvQueue);
        printf("-%p ,", (void *)process);
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
                while (scanf("%p", &pid) != 1) {
                    printf("Invalid input. Please enter a valid PID: ");
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
                quantum(&os, true, false);
                break;
            case 'S': {
                PCB* target_pid;
                char message[MAX_MESSAGE_LENGTH];
                printf("Enter PID of the process to send message to: ");
                while (scanf("%p", &target_pid) != 1) {
                    printf("Invalid input. Please enter a valid PID: ");
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
                while (scanf("%p", &reply_pid) != 1) {
                    printf("Invalid input. Please enter a valid PID: ");
                    while (getchar() != '\n'); // Clear input buffer
                }
                printf("Enter reply message (max 40 characters): ");
                scanf("%s", reply_message);
                reply(&os, reply_message);
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
                while (scanf("%p", &pid) != 1) {
                    printf("Invalid input. Please enter a valid integer PID: ");
                    while (getchar() != '\n'); // Clear input buffer
                }
                process_info(&os, pid);
                break;
            }
            case 'T':
                printf("Total Info:\n");
                total_info(&os);
                break;
            default:
                printf("Invalid command. Try again.\n");
        }

    } while (os.process_count > 0);

    return 0;
}
