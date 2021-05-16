#include "process.h"

#include "../interrupts/syscalls.h"
#include "../devices/terminal.h"
#include "../lib/lib.h"
#include "../memory/paging.h"
#include "../arch/x86_desc.h"
#include "../devices/keyboard.h"
#include "../interrupts/i8259.h"

// Terminal switching data structures
static pid_t terminal_pid_head[NUM_TERMINALS] = {(unsigned)-1, (unsigned)-1, (unsigned)-1};

// Bitmap of PIDs in use
static uint32_t pid_map = 0;

// Global counter of the number of executing tasks
int num_tasks = 0;

// Global tracker of currently active pid (initiallly invalid)
pid_t active_pid = (unsigned)-1;

// 2 basic fops tables
static struct file_ops stdin_fops = {
    .read = terminal_read,
    .write = syscall_fail,
    .open = syscall_fail,
    .close = syscall_fail
};
static struct file_ops stdout_fops = {
    .write = terminal_write,
    .read = syscall_fail,
    .open = syscall_fail,
    .close = syscall_fail
};

/* init_pcb
 * DESCRIPTION:         initializes the PCB for a new task
 * INPUTS:              pcb -- pointer to PCB struct
 *                      pid -- the process ID to assign this PCB
 *                      args -- Buffer to hold the arguments for the task
 */
void init_pcb(pcb_t* pcb, pid_t pid, char args[TERMINAL_BUF_SIZE]){
    /* Initializing the PCB entries */
    memset(pcb, 0, sizeof(pcb_t));

    pcb_t* parent_pcb = get_pcb(active_pid);

    pcb->flags = TASK_EXECUTING;
    pcb->context.esp = USER_STACK;
    pcb->context.ebp = USER_STACK;

    pcb->files[STDIN].ops = &stdin_fops;
    pcb->files[STDIN].flags = FILE_IN_USE;
    pcb->files[STDOUT].ops = &stdout_fops;
    pcb->files[STDOUT].flags = FILE_IN_USE;

    pcb->terminal = parent_pcb == NULL ? get_active_terminal() : parent_pcb->terminal;
    pcb->parent_pid = active_pid;

    /* Parsing the argument into the PCB */
    memcpy(pcb->args, args, strlen(args) + 1);
}

/* reserve_pid
 * DESCRIPTION:         reserves a pid for a new task
 * RETURNS:             the newly reserved PID (0-based), or -1 on failure
 */
pid_t reserve_pid(){
    // Find first available PID
    pid_t pid = 0;
    while(pid_map & (1 << pid)){
        ++pid;
        if(pid > MAX_PID) return (unsigned)-1;
    }

    // Flag this PID as reserved
    pid_map |= (1 << pid);
    ++num_tasks;

    return pid;
}

/* free_pid
 * DESCRIPTION:         marks the given pid as available
 * INPUTS:              pid -- the pid to free (0-based)
 * RETURNS:             0 on success, -1 on failure
 */
int free_pid(pid_t pid){
    // Check that PID is valid and process is not already free
    if(pid > MAX_PID) return -1;
    if(!(pid_map & (1 << pid))) return -1;
    --num_tasks;

    // Set as free
    pid_map &= ~(1 << pid);
    return 0;
}

/* pid_in_use
 * DESCRIPTION:         returns whether or not a PID is in use
 * INPUTS:              pid -- the PID to check
 * RETURNS:             1 if in use, 0 otherwise
 */
int pid_in_use(pid_t pid){
    return pid_map & (1 << pid);
}

/* get_pcb
 * RETURNS:             pointer to the pcb for the task with the given PID
 * INPUTS:              pid -- the PID of the task
 */
pcb_t* get_pcb(pid_t pid){
    if(pid > MAX_PID) return NULL;
    return (pcb_t*)(USER_KTASK_BASE - (pid + 1)*USER_KTASK_OFFSET);
}

/* get_kernel_stack
 * RETURNS:             address of kernel stack for this process
 * INPUTS:              pid -- the PID of the process
 */
uint32_t get_kernel_stack(pid_t pid){
    if(pid > MAX_PID) return NULL;
    return USER_KTASK_BASE - pid*USER_KTASK_OFFSET;
}

/* set_terminal_pid_head
 * DESCRIPTION:         sets which task is the most recent in the given terminal
 * INPUTS:              term -- the terminal
                        pid -- the PID of the new task
 * RETURNS:             0 on success, -1 on failure
 */
int set_terminal_pid_head(uint32_t term, pid_t pid){
    if(term >= NUM_TERMINALS) return -1;
    if(pid > MAX_PID) return -1;

    terminal_pid_head[term] = pid;
    return 0;
}

/* get_terminal_pid_head
 * RETURNS:             the PID of the most recent task in the given terminal
 * INPUTS:              term -- the terminal
 */
pid_t get_terminal_pid_head(uint32_t term){
    if(term >= NUM_TERMINALS) return -1;

    return terminal_pid_head[term];
}

/* focus_terminal
 * DESCRIPTION:         sets the given terminal as active
 * INPUT:               terminal -- the terminal to activate
 *                      context -- the context of the current task to pause
 * NOTES:               called from interrupt context, must send keyboard EOI
 */
void focus_terminal(uint32_t terminal, int_regs_t context){
    if(terminal >= NUM_TERMINALS) return;

    uint32_t prev_term = get_active_terminal();
    if(terminal == prev_term) return;

    pid_t pid = get_terminal_pid_head(terminal);
    if(pid > MAX_PID) return; // kernel.c should be setting up all initial shells

    pause_task(context);

    // Save screen state of old terminal
    memcpy(VIDEO_PTR(prev_term), VIDEO_PTR(-1), VIDEO_SIZE);

    set_terminal(terminal); // Alert terminal driver

    // Restore screen state of new terminal
    memcpy(VIDEO_PTR(-1), VIDEO_PTR(terminal), VIDEO_SIZE);
    set_screen_pos(get_terminal_x(terminal), get_terminal_y(terminal));

    send_eoi(KEYBOARD_IRQ);
    if(-1 == resume_task(pid)){
        printf("Task resumption failed for pid: %u\n", pid);
    }
}

/* pause_task
 * DESCRIPTION:         saves the current task's context
 *                      (assumes this is called from interrupt context)
 * INPUTS:              context -- the register context to save for resumption
 */
void pause_task(int_regs_t context){
    pcb_t* pcb = get_pcb(active_pid);
    pcb->context = context;

    // Save terminal position
    set_terminal_pos(pcb->terminal, get_screen_x(), get_screen_y());

    delete_task_page();
    if(pcb->flags & TASK_VID_IN_USE){
        disable_user_video_mem();
    }
}

/* resume_task
 * DESCRIPTION:         resumes the task specified task
 * INPUTS:              pid -- the task to resume
 * RETURNS:             -1 on failure (does not return on success)
 */
int resume_task(pid_t pid){
    if(pid > MAX_PID) return -1;

    pcb_t* pcb = get_pcb(pid);
    if(pcb == NULL) return -1;
    if(!(pcb->flags & TASK_EXECUTING)) return -1;

    tss.esp0 = get_kernel_stack(pid);
    setup_task_page(pid);
    if(pcb->flags & TASK_VID_IN_USE){
        setup_user_video_mem(pcb);
    }

    // Make print calls write correctly
    if(pcb->terminal == get_active_terminal()){
        set_lib_video_mem(-1);
    }
    else{
        set_lib_video_mem(pcb->terminal);
    }
    set_screen_pos(get_terminal_x(pcb->terminal), get_terminal_y(pcb->terminal));

    active_pid = pid;

    // Setup context on stack (See Intel manual figure 5-4)
    asm volatile(
        /*
            PCB context stores ESP from just after error code.
            We will move to that stack but with room for the pushed state,
            and then do the same as in "ret_from_intr" (interrupt_link.S)
        */
        "movl %0, %%esp;"
        "subl $32, %%esp;"
        "popal;"
        "addl $4, %%esp;" // pop error code
        "iret;"
        : /* no outputs */
        : "r"(pcb->context.esp)
        : "cc", "memory"
    );

    return -1; // Should never reach here
}
