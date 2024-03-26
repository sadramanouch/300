#ifndef OS_H
#define OS_H

#include "list.h" 

#define MAX_PROCESSES 10
#define MAX_MESSAGE_LENGTH 40
#define MAX_SEMAPHORES 5
#define NUM_PROCESS_QUEUE_LEVELS 3

#define Bool int
#define true 1
#define false 0

typedef enum { HIGH, NORMAL, LOW } Priority;                // for specifying priority queues
typedef enum { READY, BLOCKED, RUNNING, TERMINATED } Status;// for state of a process

// semaphore structure
typedef struct Sem {
    Bool exists;    // true when user calls N for the semaphore to create it
    int value;      
} Sem;

// process structure
typedef struct PCB {
    Priority priority; 
    Status status;
    char proc_message[MAX_MESSAGE_LENGTH];  // for send and receiving msgs
    struct PCB* sender_pid;                 // for send and receiving msgs
    Bool Turn;                              // process has had a turn on the CPU in the current round of round robin
    Bool receiving;                         // process is waiting to receive a message
    Bool sending;                           // process sent a message and is waiting for a reply
} PCB;

// operating system structure
typedef struct OS {
    int process_count;                      // total processes in the OS (at least 1 at all times)
    PCB* INIT_PROCESS_PID;                  // the PID of the initial process
    List* queues[NUM_PROCESS_QUEUE_LEVELS]; // ready queues
    List* sendQueue;                        // processes blocked on a send
    List* recvQueue;                        // processes blocked on a receive
    PCB* running_process;                   // the PID of the currently running process
    Sem* semaphores[MAX_SEMAPHORES];            // array of the semaphores
    List* semaphore_wait_queues[MAX_SEMAPHORES];// array of lists of processes blocked particular semaphores
} OS;

// free the sems and init process when shutting down
void cleanup(OS* os);

// returns the sum of the number of processes in all of the ready queues
int numReadyProcesses(OS* os);

// initializes the OS. Called ONCE upon executing
void init(OS *os);

// creates a new process
void create(OS *os, Priority priority);

// creates a new process identical to the currently running process
void forkk(OS *os);

// kills a particuar process
void kill(OS *os, PCB* target_pid);

// kills the currently running process
void exitOS(OS *os);

// kicks the currently running process (X) off the CPU and chooses a new one.
// que == true -> ready queue X, ignore kill_process
// que == false, kill_process == true -> free X after it is kicked off
// que == false, kill_process == flase -> set X->status = BLOCKED (it is in a blocking queue)
void quantum(OS *os, Bool que, Bool kill_process);

// send a message from the running process to the specified process,
// block until a reply is received
void send(OS *os, PCB* target_pid, char *msg);

// receive a message. If no one sent a message, block until one is sent.
void receive(OS *os);

// (FOR A PROCESS THAT HAS RECEIVED A MESSAGE) send a reply message
void reply(OS *os, char *reply_msg);

// create a new semaphore with the given id (0 <= id < MAX_SEMAPHORES) and initial value
void semaphore(OS *os, int semaphore_id, int initial_value);

// perform the P operation using the specified semaphore on the running process
void semaphore_P(OS *os, int semaphore_id);

// perform the V operation using the specified semaphore on the running process
void semaphore_V(OS *os, int semaphore_id);

// print information about the given process (generally, PCB struct fields)
void process_info(OS *os, PCB* pid);

// print information about the OS (generally, OS struct fields)
void total_info(OS *os);

#endif /* OS_H */
