#include "process_tests.h"
#include "tests.h"

#include "../storage/filesys.h"
#include "../interrupts/syscalls.h"
#include "../devices/terminal.h"
#include "../tasks/process.h"
#include "../lib/lib.h"

// 2 basic fops tables
static struct file_ops stdin_fops = {
    .read = terminal_read,
    .write = syscall_fail,
    .open = syscall_fail,
    .close = syscall_fail
};
static struct file_ops stdout_fops = {
    .write = terminal_write,
    .read = syscall_fail,
    .open = syscall_fail,
    .close = syscall_fail
};

// Fake PCB used for syscall tests that take place outside of a task
static pcb_t fake_pcb = {
	.files[STDIN].flags = FILE_IN_USE,
	.files[STDIN].ops = &stdin_fops,
	.files[STDOUT].flags = FILE_IN_USE,
	.files[STDOUT].ops = &stdout_fops
};

/* File fops test
 * DESCRIPTION:		tests the file_fops functions
 * RETURNS:			PASS/FAIL
 * COVERAGE: 		file_fops functions
 * FILES:		syscalls.h/c, filesys.h/c
 */
int file_fops_tests(){
    TEST_HEADER();

    active_pid = 0;
    pcb_t* pcb = get_pcb(active_pid);
    *pcb = fake_pcb;

	char* filename = "verylargetextwithverylongname.tx";
	#define EXPECTED_STR "very large text file with a very long name"
	#define EXPECTED_STR_LEN 42

	int result; // Used to keep track of return values

	// Test opening of file
	int fd;
	fd = ece391_open((uint8_t*)filename);
	if((unsigned)fd > MAX_FILES){
		printf("ece391_open produced invalid file descriptor\n");
		return FAIL;
	}

	// Test reading of file
	char buf[EXPECTED_STR_LEN + 1];
	buf[EXPECTED_STR_LEN] = '\0';
	result = ece391_read(fd, buf, EXPECTED_STR_LEN);
	if(result != EXPECTED_STR_LEN){
		printf("ece391_read did not read full nbytes\n");
		printf("Expected %d but got %d\n", EXPECTED_STR_LEN, result);
		return FAIL;
	}
	if(strncmp(EXPECTED_STR, buf, EXPECTED_STR_LEN)){
		printf("ece391_read produced incorrect data\n");
		printf("Expected \"%s\" but got \"%s\"\n", EXPECTED_STR, buf);
		return FAIL;
	}

	// Test writing of file
	if(ece391_write(fd, buf, EXPECTED_STR_LEN) != -1){
		printf("ece391_write did not fail on read-only file system\n");
		return FAIL;
	}

	// Test closing of file
	if(ece391_close(fd) != 0){
		printf("ece391_close failed\n");
		return FAIL;
	}

	// Test usage of file descriptor after close
	if(ece391_read(fd, buf, EXPECTED_STR_LEN) != -1){
		printf("ece391_read did not fail on a closed file descriptor\n");
		return FAIL;
	}

    active_pid = (unsigned)-1;
	#undef EXPECTED_STR
	#undef EXPECTED_STR_LEN
	return PASS;
}

/* Directory fops test
 * DESCRIPTION:		tests the dir_fops functions
 * RETURNS:			PASS/FAIL
 * COVERAGE: 		dir_fops functions
 * FILES:			syscalls.h/c, filesys.h/c
 */
int dir_fops_tests(){
    TEST_HEADER();

    active_pid = 0;
    pcb_t* pcb = get_pcb(active_pid);
    *pcb = fake_pcb;

	char* filename = ".";
	int counter = 0;

	#define EXPECTED_COUNT 17
	char* expected[EXPECTED_COUNT] = {
		".",
		"sigtest",
		"shell",
		"grep",
		"syserr",
		"rtc",
		"fish",
		"counter",
		"pingpong",
		"cat",
		"frame0.txt",
		"verylargetextwithverylongname.tx",
		"ls",
		"testprint",
		"created.txt",
		"frame1.txt",
		"hello"
	};

	// Test opening of file
	int fd;
	fd = ece391_open((uint8_t*)filename);
	if((unsigned)fd > MAX_FILES){
		printf("ece391_open produced invalid file descriptor\n");
		return FAIL;
	}

	// Test readings of file
	char buf[MAX_FILENAME_SIZE + 1];

	while(ece391_read(fd, buf, MAX_FILENAME_SIZE) != 0){
		if(counter > EXPECTED_COUNT){
			printf("ece391_read on directory did not fail after %d times as expected\n", EXPECTED_COUNT);
			return FAIL;
		}
		if(strncmp(expected[counter], buf, MAX_FILENAME_SIZE)){
			printf("ece391_read produced incorrect data\n");
			printf("Expected \"%s\" but got \"%s\"\n", expected[counter], buf);
			return FAIL;
		}

		// Reset for next round
		memset(buf, '\0', MAX_FILENAME_SIZE + 1);
		counter++;
	}

	if(counter != EXPECTED_COUNT){
		printf("Expected %d dir reads before failure but got %d\n", EXPECTED_COUNT, counter);
	}

	// Test usage of file descriptor after close
    ece391_close(fd);
	if(ece391_read(fd, buf, MAX_FILENAME_SIZE) != -1){
		printf("ece391_read did not fail on a closed file descriptor\n");
        printf("returns: %d\n", ece391_read(fd, buf, MAX_FILENAME_SIZE));
		return FAIL;
	}

    active_pid = (unsigned)-1;
	#undef EXPECTED_COUNT
	return PASS;
}

/* invalid fops test
 * DESCRIPTION: 		Tests that invalid fops calls fail
 * 						(e.g. read on STDOUT)
 * COVERAGE:			PCB, file_ops
 * FILES:				process.h, syscalls.h/c
 */
int invalid_fops_test(){
    TEST_HEADER();

    active_pid = 0;
    pcb_t* pcb = get_pcb(active_pid);
    *pcb = fake_pcb;

	#define BUF_SIZE 10
	int fd = STDOUT;

	char buf[BUF_SIZE];
	if(ece391_read(fd, buf, BUF_SIZE) != -1){
		printf("ece391_read on STDOUT did not fail\n");
		return FAIL;
	}

	fd = STDIN;
	if(ece391_write(fd, buf, BUF_SIZE) != -1){
		printf("ece391_write on STDIN did not fail\n");
		return FAIL;
	}

    active_pid = (unsigned)-1;
	#undef BUF_SIZE
	return PASS;
}
