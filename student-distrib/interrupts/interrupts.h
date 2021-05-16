#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "../lib/lib.h"

/* Interupt regs structure
 *  (pushed to stack by processor on interrupt,
 *   see I-32A ref figure 5-4)
 */
typedef struct {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t error_code;
    uint32_t eip;
    uint16_t cs;
    uint32_t eflags;
} int_regs_t;

extern void do_intv(int intv, int_regs_t regs);
void do_irq(int irq, int_regs_t context);
void exception_debug(int intv, int_regs_t regs);
void init_idt(void);

#endif /* INTERRUPTS_H */
