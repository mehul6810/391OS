#include "paging_tests.h"
#include "tests.h"

#include "../lib/lib.h"

/* deref
 * Inputs: a - pointer to int
 * Return: dereferenced int value at a
 */
int32_t deref(int32_t * a) {
    return *a;
}

/* write
 * Inputs: addr - pointer to int
           val - int value to write
 * Return: None
 */
void write(int32_t * addr, int32_t val) {
    *addr = val;
}

/* Invalid Dereferencing Tests
 *
 * Tests dereferencing invalid address and locations that are not accessible
 * Inputs: None
 * Outputs: None
 * Side Effects: Page fault
 * Coverage: Paging directory and table
 * Files: paging.h/c, pagingassembly.S
 */
int invalid_deref_test(){
	TEST_HEADER();

	// Dereference NULL pointer
    deref(NULL);

    // Dereference negative address
    // deref(-100);

    // Dereference unmapped region (after kernel)
    // deref(0x800001);

    // Dereference unmapped region (right befor vidmem)
    // deref(0xb7FFF);

	return FAIL;
}

/* Invalid Writing Tests
 *
 * Tests writing to invalid address and locations that are not accessible
 * Inputs: None
 * Outputs: None
 * Side Effects: Page fault
 * Coverage: Paging directory and table
 * Files: paging.h/c, pagingassembly.S
 */
int invalid_write_test(){
	TEST_HEADER();
    int val = 100;

		// Write to address NULL
    write(NULL, val);

    // Write to negative address
    // write(-100, val);

    // Write to unmapped region (after kernel)
    // write(0x800001, val);

    // Dereference unmapped region (right befor vidmem)
    // write(0xb7FFF);

	return FAIL;
}

/* Valid deference and write
 *
 * Tests writing and derferencing valid addresses and locations that are accessible
 * Side Effects: Prints out address and value inside address
 * Coverage: Paging directory and table
 * Files: paging.h/c, pagingassembly.S
 */
int valid_deref_test(){
	TEST_HEADER();

    int32_t temp;
    deref(&temp);

	return PASS;
}

/* Valid deference and write
 *
 * Tests writing and derferencing valid addresses and locations that are accessible
 * Inputs: address
 * Outputs: None
 * Side Effects: Prints out address and value inside address
 * Coverage: Paging directory and table
 * Files: paging.h/c, pagingassembly.S
 */
int valid_write_test(){
	TEST_HEADER();

    int32_t temp;
    write(&temp, 391);
    if (deref(&temp) != 391) return FAIL;

	return PASS;
}
