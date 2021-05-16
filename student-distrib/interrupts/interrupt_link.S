# Assembly linkage for intv num (w/o error code)
.macro INT_WO_ERR num:req
    .globl asm_intv_\num
    asm_intv_\num:
        # processor has already pushed EFLAGS, CS, EIP
        #  but NOT Error Code
        push $0 # push fake error code
        pushal
        pushl $\num # push intv argument
        jmp common_interrupt
.endm

# Assembly linkage for intv num (w/ error code)
.macro INT_W_ERR num:req
    .globl asm_intv_\num
    asm_intv_\num:
        # processor has already pushed EFLAGS, CS, EIP, Error Code
        pushal
        pushl $\num # push intv argument
        jmp common_interrupt
.endm

# Create interrupt linkage for applicable vectors
#  (use error code on only the applicable. See https://wiki.osdev.org/Exceptions)
INT_WO_ERR 0 # Divide Error
INT_WO_ERR 1 # Reserved
INT_WO_ERR 2 # NMI Interrupt
INT_WO_ERR 3 # Breakpoint
INT_WO_ERR 4 # Overflow
INT_WO_ERR 5 # BOUND Range Exceeded
INT_WO_ERR 6 # Invalid Opcode
INT_WO_ERR 7 # Device Not Available
INT_W_ERR 8 # Double Fault
INT_WO_ERR 9 # Coprocessor Segment (reserved)
INT_W_ERR 10 # Invalid TSS
INT_W_ERR 11 # Segment Not Present
INT_W_ERR 12 # Stack-Segment Fault
INT_W_ERR 13 # General Protection
INT_W_ERR 14 # Page Fault
INT_WO_ERR 15 # Reserved
INT_WO_ERR 16 # Floating Point Error
INT_W_ERR 17 # Alignment Check
INT_WO_ERR 18 # Machine Check
INT_WO_ERR 19 # SIMD Floating-Point
INT_WO_ERR 20 # Virtualization Exception
# 21-29 are reserved
INT_W_ERR 30 # Security Exception
# 31 is reserved
INT_WO_ERR 32 # Timer chip
INT_WO_ERR 33 # Keyboard
INT_WO_ERR 34 # Cascade to slave
INT_WO_ERR 40 # Real time clock

# See syscall_link.S for syscall linkage

common_interrupt:
    call do_intv
    addl $4, %esp # pop intv argument
ret_from_intr:
    popal
    addl $4, %esp # pop error code
    iret
