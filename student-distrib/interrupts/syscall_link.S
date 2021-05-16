#define ASM 1
#include "../arch/x86_desc.h"

#define N_SYSCALLS 10

.text

.macro SYS_SAVE_REGS
    # Don't need to save eax since it is used for return value
    pushl %ecx
    pushl %edx
    pushl %ebx
    pushl %esi
    pushl %edi
.endm

.macro SYS_RESTORE_REGS
    popl %edi
    popl %esi
    popl %ebx
    popl %edx
    popl %ecx
    # Don't need to pop eax since it's used for return value
.endm

# System Calls
.globl asm_syscall
asm_syscall:
pushl %ebp # Set up stack frame
movl %esp, %ebp
SYS_SAVE_REGS
subl $1, %eax # Make syscall num 0-based
cmpl $N_SYSCALLS, %eax # Check syscall number
jae asm_syscall_invalid
pushl %edx # push arguments (max of 3)
pushl %ecx
pushl %ebx
call *syscall_table(, %eax, 4)
addl $12, %esp # pop args
SYS_RESTORE_REGS # Doesn't affect EAX
# TODO: use static var errno
leave
iret

asm_syscall_invalid:
# If we're here, we got an invalid syscall number
SYS_RESTORE_REGS
movl $-1, %eax # Return error
leave
iret

syscall_table:
.long do_halt, do_execute, do_read, do_write, do_open, do_close, do_getargs, do_vidmap, do_set_handler, do_sigreturn

# SYSCALL LINKAGE
# ece391_* puts arguments in registers before calling int $0x80

.macro DO_SYSCALL
pushl %ebx # callee saved
# Move args to registers
movl 8(%esp), %ebx
movl 12(%esp), %ecx
movl 16(%esp), %edx
int $0x80
popl %ebx
ret
.endm

# int32_t halt(uint8_t status)
.globl ece391_halt
ece391_halt:
movl $1, %eax # halt is syscall 1
DO_SYSCALL

# int32_t execute(const uint8_t* command)
.globl ece391_execute
ece391_execute:
movl $2, %eax # execute is syscall 2
DO_SYSCALL

# int32_t read (int32_t fd, void* buf, int32_t nbytes)
.globl ece391_read
ece391_read:
movl $3, %eax # read is syscall 3
DO_SYSCALL

# int32_t write (int32_t fd, const void* buf, int32_t nbytes)
.globl ece391_write
ece391_write:
movl $4, %eax # write is syscall 4
DO_SYSCALL

# int32_t open (const uint8_t* filename)
.globl ece391_open
ece391_open:
movl $5, %eax # open is syscall 5
DO_SYSCALL

# int32_t close (int32_t fd)
.globl ece391_close
ece391_close:
movl $6, %eax # close is syscall 6
DO_SYSCALL

# int32_t getargs(uint8_t* buf, int32_t nbytes)
.globl ece391_getargs
ece391_getargs:
movl $7, %eax # getargs is syscall 7
DO_SYSCALL

# int32_t vidmap (uint8_t** screen_start)
.globl ece391_vidmap
ece391_vidmap:
movl $8, %eax # vidmap is syscall 8
DO_SYSCALL

# int32_t set_handler(int32_t signum, void* handler_address)
.globl ece391_set_handler
ece391_set_handler:
movl $9, %eax # set_handler is syscall 9
DO_SYSCALL

# int32_t sigreturn()
.globl ece391_sigreturn
ece391_sigreturn:
movl $10, %eax # sigreturn is syscall 10
DO_SYSCALL
