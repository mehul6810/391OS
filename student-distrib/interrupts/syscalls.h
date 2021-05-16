#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include "../lib/lib.h"
#include "../tasks/process.h"

int32_t syscall_fail();

// Halt status to return upon exception
#define EXCEPTION_STATUS 256

#define MAX_TASKS 6

// Global exception flag
extern volatile int32_t exception_flag;

// Called by asm linkage
extern int32_t do_halt (uint8_t status);
extern int32_t do_execute (const uint8_t* command);
extern int32_t do_read (int32_t fd, void* buf, int32_t nbytes);
extern int32_t do_write (int32_t fd, const void* buf, int32_t nbytes);
extern int32_t do_open (const uint8_t* filename);
extern int32_t do_close (int32_t fd);
extern int32_t do_getargs (uint8_t* buf, int32_t nbytes);
extern int32_t do_vidmap (uint8_t** screen_start);
extern int32_t do_set_handler (int32_t signum, void* handler);
extern int32_t do_sigreturn (void);

// Called by user, executes int 0x80
extern int32_t ece391_halt (uint8_t status);
extern int32_t ece391_execute (const uint8_t* command);
extern int32_t ece391_read (int32_t fd, void* buf, int32_t nbytes);
extern int32_t ece391_write (int32_t fd, const void* buf, int32_t nbytes);
extern int32_t ece391_open (const uint8_t* filename);
extern int32_t ece391_close (int32_t fd);
extern int32_t ece391_getargs (uint8_t* buf, int32_t nbytes);
extern int32_t ece391_vidmap (uint8_t** screen_start);
extern int32_t ece391_set_handler (int32_t signum, void* handler);
extern int32_t ece391_sigreturn (void);

// Helper functions
pid_t prep_task(const uint8_t* command);

#endif
