#include "interrupts.h"

#include "../lib/lib.h"
#include "../arch/x86_desc.h"
#include "i8259.h"
#include "../devices/keyboard.h"
#include "../devices/rtc.h"
#include "interrupt_link.h"
#include "syscall_link.h"
#include "../tasks/process.h"
#include "syscalls.h"
#include "pit.h"
#include "../scheduler/scheduler.h"

#define EXCEPTION_INFO 1

/** init_idt
 * DESCRIPTION: Initializes the IDT with handlers for the 32 exceptions
 *   and 4 notable IRQ lines (timer, keyboard, slave cascade, RTC)
 * INPUTS: none
 * OUTPUTS: none
 */
void init_idt(){
    int i; /* Iterator var */

    // Initialize IDT entries (0-31 are reserved by Intel)
    for(i = 0; i < NUM_VEC; i++){
        idt[i].seg_selector = KERNEL_CS;

        switch(i){
            // Exceptions
            case 0: SET_IDT_ENTRY(idt[i], &asm_intv_0); break;
            case 1: SET_IDT_ENTRY(idt[i], &asm_intv_1); break;
            case 2: SET_IDT_ENTRY(idt[i], &asm_intv_2); break;
            case 3: SET_IDT_ENTRY(idt[i], &asm_intv_3); break;
            case 4: SET_IDT_ENTRY(idt[i], &asm_intv_4); break;
            case 5: SET_IDT_ENTRY(idt[i], &asm_intv_5); break;
            case 6: SET_IDT_ENTRY(idt[i], &asm_intv_6); break;
            case 7: SET_IDT_ENTRY(idt[i], &asm_intv_7); break;
            case 8: SET_IDT_ENTRY(idt[i], &asm_intv_8); break;
            case 9: SET_IDT_ENTRY(idt[i], &asm_intv_9); break;
            case 10: SET_IDT_ENTRY(idt[i], &asm_intv_10); break;
            case 11: SET_IDT_ENTRY(idt[i], &asm_intv_11); break;
            case 12: SET_IDT_ENTRY(idt[i], &asm_intv_12); break;
            case 13: SET_IDT_ENTRY(idt[i], &asm_intv_13); break;
            case 14: SET_IDT_ENTRY(idt[i], &asm_intv_14); break;
            case 15: SET_IDT_ENTRY(idt[i], &asm_intv_15); break;
            case 16: SET_IDT_ENTRY(idt[i], &asm_intv_16); break;
            case 17: SET_IDT_ENTRY(idt[i], &asm_intv_17); break;
            case 18: SET_IDT_ENTRY(idt[i], &asm_intv_18); break;
            case 19: SET_IDT_ENTRY(idt[i], &asm_intv_19); break;
            case 20: SET_IDT_ENTRY(idt[i], &asm_intv_20); break;
            case 30: SET_IDT_ENTRY(idt[i], &asm_intv_30); break;

            // Hardware Interrupts
            case 32: SET_IDT_ENTRY(idt[i], &asm_intv_32); break;
            case 33: SET_IDT_ENTRY(idt[i], &asm_intv_33); break;
            case 34: SET_IDT_ENTRY(idt[i], &asm_intv_34); break;
            case 40: SET_IDT_ENTRY(idt[i], &asm_intv_40); break;

            // System Calls
            case 0x80: SET_IDT_ENTRY(idt[i], &asm_syscall); break;

            // Others
            default:
                // Set remainders to the "RESERVED" exception handler
                SET_IDT_ENTRY(idt[i], &asm_intv_1);
                break;
        }

        // Set type as Interrupt Gate 0x8E00
        idt[i].reserved4 = 0x0;
        idt[i].reserved3 = 0x0;
        idt[i].reserved2 = 0x1;
        idt[i].reserved1 = 0x1;
        idt[i].size = 0x1;
        idt[i].reserved0 = 0x0;

        idt[i].present = 0x1;
        idt[i].dpl = 0x0;
    }

    // Allow users to use syscalls (0x80)
    // Set syscall entry as trap gate
    idt[0x80].dpl = 0x3;
    idt[0x80].reserved3 = 0x1;

    // Load IDTR
    lidt(idt_desc_ptr);
}

/** do_intv
 * DESCRIPTION: C function called whenever an interrupt happens (from assembly linkage)
 * INPUTS: intv -- the interrupt vector whose linkage called this function
 *         eip -- the EIP before the interrupt happened
 * OUTPUTS: none
 * SIDE EFFECTS: Calls exception or IRQ handlers
 */
void do_intv(int intv, int_regs_t regs){
    // if(intv != 40) printf("Interrupt received with vector: %d at EIP: 0x%x\n", intv, regs.eip);

    if(intv < 32){

#if (EXCEPTION_INFO == 1)
        exception_debug(intv, regs);
#endif

        // halt current process with exception
        if(num_tasks > 0){
            exception_flag = 1;
            do_halt(0xFF); // Input doesn't matter if flag is high
        }
        else{
// Only print debug if not already done
#if (EXCEPTION_INFO == 0)
            exception_debug(intv, regs);
#endif
            printf("Exception in kernel w/o tasks. Going into while(1)\n");
            while(1);
        }
    }
    // If this is an IRQ, run do_irq
    else if(intv >= 32 && intv <= 48){
        do_irq(intv - 32, regs);
    }
}

/** do_irq
 * DESCRIPTION: Handles device IRQ interrupts
 * INPUTS: irq -- the IRQ number (0 to 16)
 *         context -- the interrupt context (register state)
 * OUTPUTS: none
 * SIDE EFFECTS: calls device-specific handler functions
 */
// #define SCHEDULER_COUTNER
void do_irq(int irq, int_regs_t context){

    switch(irq){
        case 0:
        #ifdef SCHEDULER_COUTNER
            increment_clock();
        #endif
            pit_handler(context);
            break;
        case 1:
            keyboard_irq(context);
            break;
        case 8:
            rtc_handler();
            break;
        default:
            printf("No IRQ handler for IRQ%d\n", irq);
            break;
    }

    // Send EOI
    send_eoi(irq);
}

/** exception_debug
 * DESCRIPTION: Prints processor state on exception
 * INPUTS: intv -- the interrupt vector
 *         eip -- the previous EIP before the exception
 * OUTPUTS: none
 */
void exception_debug(int intv, int_regs_t regs){
    // Print exception details
    printf("Exception: ");
    switch(intv){
        case 0:
            printf("Divide Error");
            break;
        case 1:
            printf("Reserved");
            break;
        case 2:
            printf("NMI Interrupt");
            break;
        case 3:
            printf("Breakpoint");
            break;
        case 4:
            printf("Overflow");
            break;
        case 5:
            printf("BOUND Range Exceeded");
            break;
        case 6:
            printf("Invalid Opcode");
            break;
        case 7:
            printf("Device Not Available");
            break;
        case 8:
            printf("Double Fault");
            break;
        case 9:
            printf("Coprocessor Segment (Reserved)");
            break;
        case 10:
            printf("Invalid TSS");
            break;
        case 11:
            printf("Segment Not Present");
            break;
        case 12:
            printf("Stack Segment Fault");
            break;
        case 13:
            printf("General Protection");
            break;
        case 14:
            printf("Page Fault");
            break;
        case 15:
            printf("Reserved");
            break;
        case 16:
            printf("Floating Point Error");
            break;
        case 17:
            printf("Alignment Check");
            break;
        case 18:
            printf("Machine Check");
            break;
        case 19:
            printf("SIMD Floating Point");
            break;
        default:
            printf("Unknown (Reserved)");
            break;
    }
    printf(
        " (%d) "
        " with info:\n\n"
        "edi: 0x%x\n"
        "esi: 0x%x\n"
        "ebp: 0x%x\n"
        "esp: 0x%x\n"
        "ebx: 0x%x\n"
        "edx: 0x%x\n"
        "ecx: 0x%x\n"
        "eax: 0x%x\n"
        "error_code: 0x%x\n"
        "eip: 0x%x\n"
        "cs: 0x%x\n"
        "eflags: 0x%x\n",
        intv,
        regs.edi, regs.esi, regs.ebp, regs.esp,
        regs.ebx, regs.edx, regs.ecx, regs.eax,
        regs.error_code, regs.eip, regs.cs, regs.eflags
    );
}
