#include "terminal_tests.h"
#include "tests.h"

#include "../devices/terminal.h"
#include "../lib/lib.h"

/* Terminal Read/Write
 *
 * Tests the terminal driver's read and write ioctls
 * Inputs: None
 * Outputs: None
 * Side Effects: Prints to screen, requires user input
 * Coverage: Paging directory and table
 * Files: paging.h/c, pagingassembly.S
 */
int terminal_rw_test(){
	TEST_HEADER();

	#define EXPECTED_STR_PRINT "Test!"
	#define EXPECTED_STR_READ (EXPECTED_STR_PRINT "\n")
	#define BUF_SIZE 7 // Size of EXPECTED_STR_READ + 1 for \0

	char* expected = EXPECTED_STR_READ;

	char* str = "(You may attempt to use the backspace key in the following test)\n"
				"Type \"" EXPECTED_STR_PRINT "\" below, and then press Enter:\n";

	if(terminal_open(NULL) == -1){
		printf("terminal_open failed\n");
		return FAIL;
	}

	// Test that writes complete
	if(terminal_write(NULL, str, strlen(str)) != strlen(str)){
		printf("Terminal write failed\n");
		return FAIL;
	}

	// Simulate typing
	printf("Simulating user typing...\n");
	terminal_fake_reading();
	int i;
	for(i = 0; i < BUF_SIZE - 1; i++){
		if(terminal_input(EXPECTED_STR_READ[i]) == -1){
			printf("Failed to input characters!\n");
		}
	}

	// Test that reads complete
	char buf[BUF_SIZE];
	buf[BUF_SIZE - 1] = '\0';
	if(terminal_read(NULL, buf, BUF_SIZE - 1) != BUF_SIZE - 1){
		printf("Terminal read failed\n");
		return FAIL;
	}

	// Test that reads are correct (ignore the last \n we're given)
	if(strncmp(expected, buf, BUF_SIZE) != 0){
		printf("strncmp failed with original str: \"%s\" and read buf \"%s\"\n", expected, buf);
		return FAIL;
	}

	#undef EXPECTED_STR_PRINT
	#undef EXPECTED_STR_READ
	#undef BUF_SIZE

	return PASS;
}

/* Terminal Read/Write Edge Cases
 *
 * Tests the terminal driver's read and write ioctls
 * with edge cases
 * Inputs: None
 * Outputs: None
 * Side Effects: Prints to screen, requires user input
 * Coverage: Paging directory and table
 * Files: paging.h/c, pagingassembly.S
 */
int terminal_rw_edgecase_test(){
	TEST_HEADER();

	/* Test reads past buffer limit */
	#define STR_SIZE (TERMINAL_BUF_SIZE + 2)
	char str[STR_SIZE + 1];
	memset(str, 'a', STR_SIZE - 1);
	str[STR_SIZE - 1] = '\n';
	str[STR_SIZE] = '\0';

	if(terminal_open(NULL) == -1){
		printf("terminal_open failed\n");
		return FAIL;
	}

	// Simulate typing
	terminal_fake_reading();
	int i;
	for(i = 0; i < STR_SIZE; i++){
		terminal_input(str[i]);
	}

	// Read buffer and expect NO '\n'
	char buf[STR_SIZE + 1];
	buf[STR_SIZE] = '\0';
	terminal_read(NULL, buf, STR_SIZE);

	if(strncmp(str, buf, STR_SIZE) == 0) return FAIL;

	#undef STR_SIZE

	/* Test writes past buffer size */
	char* str2 = "test string";
	char* str2_overflow = " **Overflow as expected**\n";
	int str2_write_len = strlen(str2) + 1 + strlen(str2_overflow);
	if(terminal_write(NULL, str2, str2_write_len) != str2_write_len){
		printf("Write past buffer size failed.\n");
		return FAIL;
	};

	return PASS;
}
