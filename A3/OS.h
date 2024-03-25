#ifndef OS_H
#define OS_H

#include "list.h" 

#define MAX_PROCESSES 10
#define MAX_MESSAGE_LENGTH 40
#define MAX_SEMAPHORES 5
#define NUM_PROCESS_QUEUE_LEVELS 3

typedef enum { HIGH, NORMAL, LOW } Priority;
typedef enum { READY, BLOCKED, RUNNING, TERMINATED } Status;

typedef struct Sem {
    bool exists;
    int value;
} Sem;

typedef struct PCB {
    Priority priority;
    Status status;
    char proc_message[MAX_MESSAGE_LENGTH];
    struct PCB* sender_pid = NULL;
    bool Turn;
} PCB;

typedef struct OS {
    int process_count;
    PCB* INIT_PROCESS_PID;
    List* queues[NUM_PROCESS_QUEUE_LEVELS];
    List* sendQueue;
    List* recvQueue;
    PCB* running_process;
    Sem* semaphores[MAX_SEMAPHORES];
    List* semaphore_wait_queues[MAX_SEMAPHORES];
} OS;

void init(OS *os);
void create(OS *os, Priority priority);
void forkk(OS *os);
void kill(OS *os, PCB* target_pid);
void exitOS(OS *os);
void quantum(OS *os, bool que, bool kill_process);
void send(OS *os, PCB* target_pid, char *msg);
void receive(OS *os);
void reply(OS *os, char *reply_msg);
void semaphore(OS *os, int semaphore_id, int initial_value);
void semaphore_P(OS *os, int semaphore_id);
void semaphore_V(OS *os, int semaphore_id);
void process_info(OS *os, PCB* pid);
void total_info(OS *os);

#endif /* OS_H */
