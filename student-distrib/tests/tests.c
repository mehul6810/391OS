#include "tests.h"

#include "../arch/x86_desc.h"
#include "../lib/lib.h"
#include "../devices/terminal.h"
#include "../storage/filesys.h"

#define TEST_OUTPUT(name, result)	\
printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

/* Checkpoint 1 tests */
#include "idt_tests.h"
#include "keyboard_tests.h"
#include "paging_tests.h"

/* Checkpoint 2 tests */
#include "filesys_tests.h"
#include "terminal_tests.h"
#include "rtc_tests.h"

/* Checkpoint 3 tests */
#include "process_tests.h"

/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	int failures = 0; /* Used to keep track of the number of failures */
	int skipped = 0; /* Used to keep track of number of skipped tests */
	int result; /* Saves each test result temporarily */

	// Define new macro which wraps the TEST_OUTPUT macro for cleaner
	// 	code. Also adds ability to count number of failures more easily!
	#define TEST(name, ...) 			\
	do{ 								\
		result = name(__VA_ARGS__);		\
		failures += (result == FAIL);	\
		TEST_OUTPUT(#name, result);		\
	}while(0)

	// Used for yes/no prompts to skip tests
	char input[2];
	file_t fake_file;

	// Define new macro for optional tests
	#define OPT_TEST(name, ...) 					     	\
	do{													    \
		printf("Run " #name "? y/[n]: ");					\
		terminal_read(&fake_file, input, 2);				\
		while(input[0] != 'y' && input[0] != 'n' 			\
				&& input[0] != '\n'){						\
			printf("Please answer yes or no. y/[n]: ");		\
			terminal_read(&fake_file, input, 2);			\
		}													\
		if(input[0] == 'y'){								\
			TEST(name, __VA_ARGS__);						\
		}													\
		else{												\
			skipped++;										\
		}													\
	} while(0)

	TEST(idt_null_test);
	TEST(idt_device_test);
	TEST(scancode_test);

    // Test dereferencing address in kernel space
	TEST(valid_deref_test);

    // Test writing to kernel space
	TEST(valid_write_test);

	// Test terminal rw
	TEST(terminal_rw_test);
	TEST(terminal_rw_edgecase_test);

	// Test file system
	TEST(read_dentry_root_test);
	TEST(read_dentry_file_test);
	TEST(read_data_test);

	// This test prints a lot, so confirm intent
	printf("The following test prints a lot. ");
	OPT_TEST(read_dir_test);

	// Next test clears screen and takes a while, so confirm intent
	printf("The following test clears the screen. ");
	OPT_TEST(rtc_r_w_test);

	TEST(file_fops_tests);
	TEST(dir_fops_tests);

	TEST(invalid_fops_test);

	printf(
		"\n"
		"********************************************************************\n"
		"Test Results:\n"
		"Test Skipped: %d\n"
		"Tests Failed: %d\n"
		"********************************************************************\n"
		"\n", skipped, failures
	);

	/* Tests below will cause exceptions */

	// Exceptions outside a task cause screen of death
	// TEST(idt_exception_test);

    // Dereferencing invalid addresses will page fault
    // TEST(invalid_deref_test);

	// Writing to invalid addresses will page fault
	// TEST(invalid_write_test);

	#undef TEST
}
