#ifndef _PROCESS_H
#define _PROCESS_H

#include "../storage/filesys.h"
#include "../devices/terminal.h"
#include "../interrupts/interrupts.h"

#define MAX_FILES 8
#define STDIN 0
#define STDOUT 1

// PCB flags
#define TASK_VID_IN_USE 1
#define TASK_EXECUTING 2
#define TASK_WAITING_FOR_CHILD 4

// Max PID is 31 because a 32-bit bitmap is used
#define MAX_PID 31
typedef uint32_t pid_t;

typedef struct pcb_struct{
    file_t files[MAX_FILES];
    char args[TERMINAL_BUF_SIZE]; // Buffer to hold the arguments
    uint32_t flags;

    pid_t parent_pid;

    uint32_t terminal;
    int_regs_t context;
} pcb_t;

// Global counter of the number of executing tasks
extern int num_tasks;

// Global variable for current process' PID
extern pid_t active_pid;


void init_pcb(pcb_t* pcb, pid_t pid, char args[TERMINAL_BUF_SIZE]);
pid_t reserve_pid();
int free_pid(pid_t pid);
int pid_in_use(pid_t pid);
pcb_t* get_pcb(pid_t pid);
uint32_t get_kernel_stack(pid_t pid);

void focus_terminal(uint32_t terminal, int_regs_t context);
int set_terminal_pid_head(uint32_t term, pid_t pid);
pid_t get_terminal_pid_head(uint32_t term);
void pause_task(int_regs_t context);
int resume_task(pid_t pid);

#endif /* _PROCESS_H */
