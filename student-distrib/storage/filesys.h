#ifndef _FILESYS_H
#define _FILESYS_H

#include "../lib/lib.h"
#include "../multiboot.h"

#define BLOCK_SIZE 4096 // 4kB

struct file_ops;
struct file_t;

// File jump table
struct file_ops {
    int32_t (*open)(struct file_t*);
    int32_t (*read)(struct file_t*, void*, int32_t);
    int32_t (*write)(struct file_t*, const void*, int32_t);
    int32_t (*close)(struct file_t*);
};

// PCB file entry struct
typedef struct file_t {
    struct file_ops* ops;
    int32_t inode;
    int32_t fpos;
    int32_t flags;
} file_t;

// file_t's flags options
#define FILE_IN_USE 0x1

#define MAX_FILENAME_SIZE 32
#define DENTRY_TYPE_RTC 0
#define DENTRY_TYPE_DIR 1
#define DENTRY_TYPE_FILE 2

// Boot block directory entry struct
typedef struct {
    int8_t name[MAX_FILENAME_SIZE];
    int32_t type;
    int32_t inode;
    int8_t reserved[24];
} dentry_t;

// Boot block struct
#define MAX_DENTRIES 63
typedef struct {
    int32_t n_dentries;
    int32_t n_inodes;
    int32_t n_blocks;
    int8_t reserved[52];
    dentry_t dentries[MAX_DENTRIES];
} boot_block_t;

// File system inode struct
typedef struct {
    int32_t size;
    int32_t block_nums[BLOCK_SIZE - 4];
} inode_t;

/* Function prototypes, definitions are in filesys.c */
void filesys_init(module_t* module);
int32_t file_open(file_t* file);
int32_t file_read(file_t* file, void* buf, int32_t n_bytes);
int32_t file_write(file_t* file, const void* buf, int32_t n_bytes);
int32_t file_close(file_t* file);
int32_t read_dentry_by_name (const int8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);
int32_t dir_read(file_t* file, void* buf, int32_t n_bytes);
int32_t dir_write(file_t* file, const void* buf, int32_t n_bytes);
int32_t dir_open(file_t* file);
int32_t dir_close(file_t* file);

/* Helpers */
inode_t* inode_at(uint32_t index);
uint8_t* data_at(uint32_t index);

#endif /* _FILESYS_H */
