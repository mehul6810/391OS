
#include "scheduler.h"

#include "../arch/x86_desc.h"
#include "../tasks/process.h"
#include "../devices/terminal.h"
#include "../memory/paging.h"
#include "../interrupts/i8259.h"
#include "../interrupts/pit.h"

/* pit_handler
 * DESCRIPTION:         PIT interrupt handler, calls scheduler
 * INPUTS:              context -- the interrupt context (register state)
 * RETURNS:             none
 */
void pit_handler(int_regs_t context) {
    // If no active processes, do nothing
    if (active_pid == -1) return;

    // Find PID of next running process
    pid_t next_pid = get_next_pid(active_pid);
    if (next_pid > MAX_PID) return;

    pause_task(context);
    send_eoi(PIT_IRQ);
    resume_task(next_pid);

    // Should never reach here
    return;
}

/* get_next_pid
 * DESCRIPTION:         helper to find next process in round-robin policy
 * INPUTS:              active_pid -- pid of current process
 * RETURNS:             pid of next process
 */
pid_t get_next_pid(pid_t old_pid) {

    pid_t pid = old_pid + 1;
    pcb_t* candidate_pcb;

    while (pid != old_pid) {
        if (pid > MAX_PID) pid = 0;
        if(pid_in_use(pid)){
            candidate_pcb = get_pcb(pid);
            if (candidate_pcb->flags & TASK_EXECUTING) {
                return pid;
            }
        }

        pid++;
    }

    return (unsigned)-1;
}
