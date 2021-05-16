#include "filesys_tests.h"
#include "tests.h"

#include "../storage/filesys.h"
#include "../lib/lib.h"

/* Read dentry test
 *
 * Reading root dentry from the disk image
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: Filesystem
 * Files: filesys.h/c
 */
int read_dentry_root_test(){
	TEST_HEADER();

	char* filename = ".";
	dentry_t dentry_index;
	dentry_t dentry_name;

	// Test read of both types
	if(read_dentry_by_index(0, &dentry_index) == -1){
		printf("read_dentry_by_index failed\n");
		return FAIL;
	}
	if(read_dentry_by_name(filename, &dentry_name) == -1){
		printf("read_dentry_by_name failed\n");
		return FAIL;
	}

	// Check filename of both types
	if(strncmp(dentry_index.name, filename, MAX_FILENAME_SIZE)){
		printf("Incorrect filename in dentry by index\n");
		printf("Expected \"%s\", but got \"%s\"\n", filename, dentry_index.name);
		return FAIL;
	}
	if(strncmp(dentry_name.name, filename, MAX_FILENAME_SIZE)){
		printf("Incorrect filename in dentry by name\n");
		printf("Expected \"%s\", but got \"%s\"\n", filename, dentry_name.name);
		return FAIL;
	}

	// Check dentry type for both
	if(dentry_index.type != DENTRY_TYPE_DIR){
		printf("Incorrect dentry type for dentry by index\n");
		printf("Expected %d but got %d\n", DENTRY_TYPE_DIR, dentry_index.type);
		return FAIL;
	}
	if(dentry_name.type != DENTRY_TYPE_DIR){
		printf("Incorrect dentry type for dentry by name\n");
		printf("Expected %d but got %d\n", DENTRY_TYPE_DIR, dentry_name.type);
		return FAIL;
	}

	return PASS;
}

/* Read dentry test
 *
 * Reading non-root dentry from the disk image
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: Filesystem
 * Files: filesys.h/c
 */
int read_dentry_file_test(){
	TEST_HEADER();

	char* filename = "frame0.txt";
	dentry_t dentry;

	// Get dentry
	if(read_dentry_by_name(filename, &dentry) == -1){
		printf("read_dentry_by_name failed\n");
		return FAIL;
	}

	// Check filename
	if(strncmp(dentry.name, filename, MAX_FILENAME_SIZE)){
		printf("Incorrect filename\n");
		printf("Expected \"%s\", but got \"%s\"\n", filename, dentry.name);
		return FAIL;
	}

	// Check dentry type
	if(dentry.type != DENTRY_TYPE_FILE){
		printf("Incorrect dentry type\n");
		printf("Expected %d but got %d\n", DENTRY_TYPE_FILE, dentry.type);
		return FAIL;
	}

	return PASS;
}

/* Dentry read_data test
 *
 * Reading data from a file dentry
 *  (Tests normal use, edge cases, and executables)
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: Filesystem
 * Files: filesys.h/c
 */
int read_data_test(){
	TEST_HEADER();

	char* filename = "verylargetextwithverylongname.tx";
	dentry_t dentry;

	// Get dentry
	if(read_dentry_by_name(filename, &dentry) == -1){
		printf("read_dentry_by_name failed\n");
		return FAIL;
	}

	// Check filename
	if(strncmp(dentry.name, filename, MAX_FILENAME_SIZE)){
		printf("Incorrect filename\n");
		printf("Expected \"%s\", but got \"%s\"\n", filename, dentry.name);
		return FAIL;
	}

	// Check dentry type
	if(dentry.type != DENTRY_TYPE_FILE){
		printf("Incorrect dentry type\n");
		printf("Expected %d but got %d\n", DENTRY_TYPE_FILE, dentry.type);
		return FAIL;
	}

	// Check basic read_data
	#define EXPECTED_STR "very large text file with a very long name"
	#define STR_LEN 42
	#define FILE_OFFSET 0

	int8_t buf[STR_LEN + 1];
	buf[STR_LEN] = '\0';
	if(read_data(dentry.inode, FILE_OFFSET, (uint8_t*)buf, STR_LEN) != STR_LEN){
		printf("read_data didn't read full length\n");
		return FAIL;
	}

	if(strncmp(EXPECTED_STR, buf, STR_LEN)){
		printf("read_data returned incorrect data\n");
		printf("Expected \"%s\", but got \"%s\"\n", EXPECTED_STR, buf);
		return FAIL;
	}

	#undef EXPECTED_STR
	#undef STR_LEN
	#undef FILE_OFFSET

	// Check multi-block read_data with offset
	#define EXPECTED_STR "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"
	#define STR_LEN 52
	#define FILE_OFFSET 0xFBD // From hexdump of file, is just before end of a block

	int8_t buf2[STR_LEN + 1];
	buf2[STR_LEN] = '\0';
	if(read_data(dentry.inode, FILE_OFFSET, (uint8_t*)buf2, STR_LEN) != STR_LEN){
		printf("read_data didn't read full length in edgecase\n");
		return FAIL;
	}

	if(strncmp(EXPECTED_STR, buf2, STR_LEN)){
		printf("read_data returned incorrect data in edgecase\n");
		printf("Expected \"%s\", but got \"%s\"\n", EXPECTED_STR, buf2);
		return FAIL;
	}

	#undef EXPECTED_STR
	#undef STR_LEN
	#undef FILE_OFFSET

	// Check reading of executables
	#define ELF_OFFSET 1
	#define ELF_SIZE 4 // "ELF" + null
	#define MAGIC_OFFSET 0x14c0 // For ls program specifically
	#define MAGIC "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	#define MAGIC_SIZE 37 // magic + null
	char* exe_name = "ls";

	// Get dentry
	if(read_dentry_by_name(exe_name, &dentry) == -1){
		printf("read_dentry_by_name failed for executable\n");
		return FAIL;
	}

	// Search for "ELF"
	int8_t elf_buf[ELF_SIZE];
	elf_buf[ELF_SIZE] = '\0';
	if(read_data(dentry.inode, ELF_OFFSET, (uint8_t*)elf_buf, ELF_SIZE) != ELF_SIZE){
		printf("read_data didn't read full length for executable\n");
		return FAIL;
	}

	// Verify ELF appears in data
	if(strncmp(elf_buf, "ELF", ELF_SIZE - 1)){
		printf("Did not find \"ELF\" at offset %d in executable \"%s\"\n", ELF_OFFSET, exe_name);
		return FAIL;
	}

	// Search for magic
	int8_t magic_buf[MAGIC_SIZE];
	magic_buf[MAGIC_SIZE] = '\0';
	if(read_data(dentry.inode, MAGIC_OFFSET, (uint8_t*)magic_buf, MAGIC_SIZE) != MAGIC_SIZE){
		printf("read_data didn't read full length for executable\n");
		return FAIL;
	}

	// Verify ELF appears in data
	if(strncmp(magic_buf, MAGIC, MAGIC_SIZE - 1)){
		printf("Did not find \"%s\" at offset %d in executable \"%s\"\n", MAGIC, MAGIC_OFFSET, exe_name);
		return FAIL;
	}

	#undef ELF_SIZE
	#undef ELF_OFFSET
	#undef MAGIC_OFFSET
	#undef MAGIC

	return PASS;
}

/* Directory read/write tests
 *
 * Tests reading filenames of directories
 * Inputs: None
 * Outputs: None
 * Side Effects: Prints to screen
 * Coverage: File system driver
 * Files: filesys.h/c
 */
int read_dir_test(){
	TEST_HEADER();

	int counter = 0;
	#define EXPECTED_COUNT 17

	char* filename = ".";
	dentry_t dentry_name;

	// Test read of both types
	if(read_dentry_by_name(filename, &dentry_name) == -1){
		printf("read_dentry_by_name failed\n");
		return FAIL;
	}

	file_t file;
	file.inode = dentry_name.inode;
	file.fpos = 0;

	uint8_t buf[MAX_FILENAME_SIZE+1];
	buf[MAX_FILENAME_SIZE] = '\0';
	while(0 != dir_read(&file, buf, MAX_FILENAME_SIZE)){
		printf("Filename: %s\n", buf);
		counter++;
	}

	if(counter == EXPECTED_COUNT)
		return PASS;
	else
		return FAIL;
	#undef EXPECTED_COUNT
}
