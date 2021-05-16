#ifndef _PAGING_H
#define _PAGING_H

#include "../tasks/process.h"

void pit_handler(int_regs_t context);
pid_t get_next_pid(pid_t pid);

#endif
