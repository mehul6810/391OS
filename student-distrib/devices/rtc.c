#include "rtc.h"
#include "keyboard.h"

#include "../lib/types.h"

/* Referring https://wiki.osdev.org/RTC#Setting_the_Registers */
#define REG_A 0x8A
#define REG_B 0x8B
#define REG_C 0x8C

/* Port Locs */
#define RTC_PORT 0x70
#define CMOS_PORT 0x71

/* RTC IRQ vector number */
#define RTC_IRQ 8

/* Defining permittible RTC frequencies */
#define FREQ_2Hz 2
#define FREQ_4Hz 4
#define FREQ_8Hz 8
#define FREQ_16Hz 16
#define FREQ_32Hz 32
#define FREQ_64Hz 64
#define FREQ_128Hz 128
#define FREQ_256Hz 256
#define FREQ_512Hz 512
#define FREQ_1024Hz 1024

/*
 *	Defining the rates for the different frequencies
 *	Reference - https://wiki.osdev.org/RTC#Changing_Interrupt_Rate
 */
#define RATE_2Hz 0x0F
#define RATE_4Hz 0x0E
#define RATE_8Hz 0x0D
#define RATE_16Hz 0x0C
#define RATE_32Hz 0x0B
#define RATE_64Hz 0x0A
#define RATE_128Hz 0x09
#define RATE_256Hz 0x08
#define RATE_512Hz 0x07
#define RATE_1024Hz 0x06	/* kernel should limit to 1024 Hz */

/* The number of bytes rtc_bytes must always receive */
#define WRITE_BYTES 4

/* RTC driver will use file->inode for virtualized frequency
   and file->fpos for a counter */
static volatile uint32_t num_interrupts = 0;

// REG_C must be read from to continue receiving interrupts
// Reference - https://wiki.osdev.org/RTC#Interrupts_and_Register_C
#define CLEAR_C()					\
do{									\
	cli();							\
	outb(REG_C, RTC_PORT);			\
	inb(CMOS_PORT);					\
	sti();							\
} while(0)

/* rtc_init
 * DESCRIPTION: 	Initializes rtc
 * SIDE EFFECTS: 	rtc interrupt enabled (slave)
 */
void rtc_init(void)
{
	uint8_t prev;

	/* ----- Setting up periodic interrupts ----- */

	/* Critical section - disable interrupts */
	cli();

	/* select Status Register A, and disable NMI */
	outb(REG_A, RTC_PORT);
	prev = inb(CMOS_PORT);
	outb(REG_B, RTC_PORT);
	outb(prev | 0x40 , CMOS_PORT);

	// RTC defaults to 1024 Hz

	CLEAR_C();

	/* Re-enabling the RTC */
	enable_irq(RTC_IRQ);
}

/* rtc_handler
 * DESCRIPTION: 	used to handle rtc interrupt calls
 * SIDE EFFECTS: 	increments interrupt counter
 */
void rtc_handler(void)
{
	num_interrupts++;
	CLEAR_C();
}

/* rtc_open
 * DESCRIPTION: 	used to reset rtc frequency to 2 Hz
 * INPUTS: 			file  -- pointer to file struct
 * RETURN VALUE: 	0 on success, -1 on failure
 * SIDE EFFECTS: 	Frequency of RTC is reset to 2 Hz
 */
int32_t rtc_open(file_t* file)
{
	if(file == NULL) return -1;

	// Setting the interrupt frequency to 2Hz (in the virtualized RTC)
	file->inode = FREQ_2Hz;

	return 0;
}

/* rtc_read
 * DESCRIPTION: 	Checks if an RTC interrupt is encountered
 * INPUTS: 			file -- pointer to file_t
 *					buf -- unused
 *					nbytes -- unused
 * OUTPUTS: 		Returns 0 when a virtual RTC interrupt is encountered
 * SIDE EFFECTS: 	none
 */
int32_t rtc_read(file_t* file, void* buf, int32_t nbytes)
{
	if(file == NULL) return -1;

	// Mark the current number of interrupts
	file->fpos = num_interrupts;

	uint32_t period = (uint32_t)(FREQ_1024Hz / file->inode);

	while (num_interrupts - (uint32_t)file->fpos < period);

	return 0;
}

/* rtc_write
 * DESCRIPTION: 	Changes the frequency of the RTC (virtualized)
 * INPUTS: 			file 		-> Pointer to file_t struct
 *					buf 		->  Holds the value of the new frequency
 *		   			nbytes 		->	Number of bytes that are being inputted (Needs to always be 4)
 * OUTPUTS: 		Number of bytes written
 * SIDE EFFECTS: 	Changes the frequency of the RTC (virtualized)
 */
int32_t rtc_write(file_t* file, const void* buf, int32_t nbytes)
{
	/* Checking if the number of input bytes is 4 (Needs to always be 4) */
	if(nbytes != WRITE_BYTES) return -1;

	/* Checking if pointers are invalid */
	if (file == NULL || buf == NULL) return -1;

	/* Variable to hold the bytes value */
	uint32_t byte_holder = *(uint32_t *)buf;

	// TODO: Should we check for this? -> After virtualization any frequency (upto 1024 Hz) should be supported
	// Require that frequency be power of 2
	if (((byte_holder != 0) && ((byte_holder & (byte_holder - 1)) != 0)))
		return -1;

	// Require that virtual frequency be less than actual frequency
	if(byte_holder > FREQ_1024Hz) return -1;

	file->inode = byte_holder;

	/* Successful, sending number of bytes written */
	return nbytes;
}

/* rtc_close
 * DESCRIPTION: 	Returns 0, RTC meant to never be closed
 * INPUTS: 			file -- pointer to file_t (unused)
 * OUTPUTS: 		Returns 0 always
 * SIDE EFFECTS: 	none
 */
int32_t rtc_close(file_t* file)
{
	// Nothing to do here
	return 0;
}
