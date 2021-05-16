/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "../lib/lib.h"

/* Data ports */
#define MASTER_8259_DATA MASTER_8259_PORT + 1
#define SLAVE_8259_DATA SLAVE_8259_PORT + 1

/* Line used for cascading */
#define IRQ_SLAVE_LINE 2

/* Number of vectors in the PIC */
#define IRQ_VEC_NUM 16

/* Offset used to address the irq vectors in slave -> 8 */
#define IRQ_OFFSET 8

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/** i8259_init
 * DESCRIPTION: Initializes the 8259 PIC
 * INPUTS: none
 * OUTPUTS: none
 */
void i8259_init(void) {

	master_mask = 0xFF;
	slave_mask = 0xFF;

	/* Sending mask data */
	outb(master_mask, MASTER_8259_DATA);
	outb(slave_mask, SLAVE_8259_DATA);

	/* Sending ICW1 */
	outb(ICW1, MASTER_8259_PORT);
	outb(ICW1, SLAVE_8259_PORT);

	/* Sending ICW2 : Initializing with offset */
	outb(ICW2_MASTER, MASTER_8259_DATA);
	outb(ICW2_SLAVE, SLAVE_8259_DATA);

	/* Sending ICW3 */
	outb(ICW3_MASTER, MASTER_8259_DATA);
	outb(ICW3_SLAVE, SLAVE_8259_DATA);

	/* Sending ICW4 */
	outb(ICW4, MASTER_8259_DATA);
	outb(ICW4, SLAVE_8259_DATA);

	/* Enabling the slave line */
	enable_irq(IRQ_SLAVE_LINE);
}

/** enable_irq
 * DESCRIPTION: Enables (unmasks) the specified IRQ
 * INPUTS: irq_num -- the irq number to unmask
 * OUTPUTS: none
 * SIDE_EFFECTS: unmasks that irq
 */
void enable_irq(uint32_t irq_num) {

	/* Checking if IRQ_num is valid : only 16 vectors are possible */
	if(irq_num < 0 || irq_num > (IRQ_VEC_NUM - 1))
		return;

	/* Checking if the IRQ is in the master */
	if (irq_num < IRQ_OFFSET)
	{
		/* Updating the master mask to enable the encountered irq */
		master_mask = master_mask & (~(1 << irq_num));
		outb(master_mask, MASTER_8259_DATA);

	} else
	/* Checking if the IRQ is in the slave */
	{
		/* Updating the slave mask to enable the encountered irq */
		slave_mask = slave_mask & (~(1 << (irq_num - IRQ_OFFSET)));
		outb(slave_mask, SLAVE_8259_DATA);
	}
}
/** disable_irq
 * DESCRIPTION: Disables (masks) the specified IRQ
 * INPUTS: irq_num -- the irq number to mask
 * OUTPUTS: none
 * SIDE_EFFECTS: masks that irq
 */
void disable_irq(uint32_t irq_num) {

	/* Checking if IRQ_num is valid : only 15 vectors are possible */
	if(irq_num < 0 || irq_num > (IRQ_VEC_NUM - 1))
		return;

	/* Checking if the IRQ is in the master */
	if (irq_num < IRQ_OFFSET)
	{
		/* Updating the master mask to disable the encountered irq */
		master_mask = master_mask | ((1 << irq_num));
		outb(master_mask, MASTER_8259_DATA);

	} else
	/* Checking if the IRQ is in the slave */
	{
		/* Updating the slave mask to disable the encountered irq */
		slave_mask = slave_mask | ((1 << (irq_num - IRQ_OFFSET)));
		outb(slave_mask, SLAVE_8259_DATA);
	}

}

/** send_eoi
 * DESCRIPTION: Send EOI signal to PIC
 * INPUTS: irq_num -- the irq number which ended
 * OUTPUTS: none
 */
void send_eoi(uint32_t irq_num) {

	/* Checking if IRQ_num is valid : only 15 vectors are possible */
	if(irq_num < 0 || irq_num > (IRQ_VEC_NUM - 1))
		return;

    // EOI must be sent to either master or both
    if(irq_num >= IRQ_OFFSET){
        outb(EOI | (irq_num - IRQ_OFFSET), SLAVE_8259_PORT);
        outb(EOI | IRQ_SLAVE_LINE, MASTER_8259_PORT);
    }
    else{
        outb(EOI | irq_num, MASTER_8259_PORT);
    }
}
