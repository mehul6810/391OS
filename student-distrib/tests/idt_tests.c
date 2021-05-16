#include "idt_tests.h"
#include "tests.h"

#include "../lib/lib.h"
#include "../arch/x86_desc.h"

/* IDT Null Test - Example
 *
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_null_test(){
	TEST_HEADER();

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) &&
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* IDT Device Tests
 *
 * Asserts that the IRQ interrupts work
 * Inputs: None
 * Outputs: PASS (failure will crash)
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: interrupts.h/c, interrupt_link.S
 */
int idt_device_test(){
	TEST_HEADER();
	int result = PASS;

	// Simulates timer
	asm volatile("int $32");

	// Simulates keyboard
	asm volatile("int $33");

	// Simulates RTC
	asm volatile("int $40");

	return result;
}

/* IDT Exception Tests
 *
 * Asserts that exceptions work
 * Inputs: None
 * Outputs: None
 * Side Effects: Stalls in a screen of death
 * Coverage: Load IDT, IDT definition
 * Files: interrupts.h/c, interrupt_link.S
 */
int idt_exception_test(){
	TEST_HEADER();
	int result = PASS;

	// This call will print a screen of death
	//  and stall in a while(1) loop
	asm volatile ("int $0");

	return result;
}
