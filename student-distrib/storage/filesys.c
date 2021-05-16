#include "filesys.h"

/* boot block pointer and filesystem size variables */
static boot_block_t* disk_img;
static int32_t disk_size;

/** filesys_init
 * DESCRIPTION: Initializes boot block pointer and size variable
 * INPUTS: module - First module being loaded in kernel
 * OUTPUTS: none
 */
void filesys_init(module_t* module){
    disk_img = (boot_block_t*)module->mod_start;
    disk_size = module->mod_end - module->mod_start;
}

/** file_open
 * DESCRIPTION:     Does nothing at the moment
 * INPUTS:          file -- pointer to file_t (unused)
 * RETURN VALUE:    0 on success, -1 on failure
 */
int32_t file_open(file_t* file){
    // No initialization must be done for a generic file open
    return 0;
}

/** file_read
 * DESCRIPTION: read a specified number of bytes from file
 * INPUTS: file - pointer to file in PCB
 * OUTPUTS: int32_t - number of bytes read, -1 on failure
 */
int32_t file_read(file_t* file, void* buf, int32_t n_bytes){
    // Check that pointers are not NULL
    if(file == NULL || buf == NULL) return -1;

    // Read data
    int32_t bytes_read = read_data(file->inode, file->fpos, buf, n_bytes);
    // TODO: expect NULL terminated string?
    // ((uint8_t*)buf)[bytes_read] = '\0';
    file->fpos += bytes_read;
    return bytes_read;
}

/** file_write
 * DESCRIPTION:     Does nothing, system is read-only
 * INPUTS:          (all unused)
 * RETURN VALUE:    -1 on failure
 */
int32_t file_write(file_t* file, const void* buf, int32_t n_bytes){
    // Read-only file system
    return -1;
}

/** file_close
 * DESCRIPTION:     Does nothing at the moment
 * INPUTS:          file -- pointer to file_t (unused)
 * RETURN VALUE:    0 on success, -1 on failure
 */
int32_t file_close(file_t* file){
    // Nothing to do here
    return 0;
}

/** read_dentry_by_name
 * DESCRIPTION: Finds dentry in boot block with given file name and populates
 * the passed in dentry struct
 * INPUTS: fname - pointer to file name
 *         dentry - dentry struct to be populated
 * OUTPUTS: int32_t return 0 on success
 */
int32_t read_dentry_by_name (const int8_t* fname, dentry_t* dentry){
    // Check that pointer is valid
    if(fname == NULL || dentry == NULL) return -1;

    // Go through each dentry to search for filename
    int i;
    for(i = 0; i < disk_img->n_dentries; i++){
        if(strncmp(fname, disk_img->dentries[i].name, MAX_FILENAME_SIZE) == 0){
            *dentry = disk_img->dentries[i];
            return 0;
        }
    }

    // File was not found
    return -1;
}

/** read_dentry_by_index
 * DESCRIPTION: Finds dentry at index and populates the passed in dentry struct
 * INPUTS: index - index of dentry in boot block
 *         dentry - dentry struct to be populated
 * OUTPUTS: int32_t return 0 on success
 */
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry){
    // Check if index is in range
    if(index >= disk_img->n_dentries) return -1;

    // Check that pointer is valid
    if(dentry == NULL) return -1;

    *dentry = disk_img->dentries[index];
    return 0;
}

/** read_data
 * DESCRIPTION: Reads specified number of bytes from file system with a given
 * inode and offset into buffer
 * INPUTS: inode - inode index to look up
 *         offset - byte offset from start of inode
 *         buf - buffer to copy data into
 *         length - number of bytes to read
 * OUTPUTS: int32_t return number of bytes read
 */
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    // Check that pointer is valid
    if(buf == NULL) return -1;

    // Find the appropriate block
    inode_t* inode_block = inode_at(inode);

    // Determine which data block to read
    int32_t data_index = offset / BLOCK_SIZE;
    int32_t data_offset = offset % BLOCK_SIZE;

    // Copy until length is done or we reach EOF
    int i = 0;
    while(i < length && i + offset < inode_block->size){
        uint8_t* data_block = data_at(inode_block->block_nums[data_index]);
        int32_t n_bytes =  min(BLOCK_SIZE - data_offset, length - i);
        memcpy(buf + i, data_block + data_offset, n_bytes);

        // Set up for next copy cycle if required
        data_offset = 0;
        data_index ++;
        i += n_bytes;
    }
    return i;
}

/** dir_read
 * DESCRIPTION: reads specified number of bytes from a given directory file
 * INPUTS: file - pointer to file in PCB
 *         buf - buffer to copy into
 *         n_bytes - number of bytes to read
 * RETURN VALUE: number of bytes read, -1 on failure
 */
int32_t dir_read(file_t* file, void* buf, int32_t n_bytes){
    if(file == NULL || buf == NULL) return -1;

    int bytes_copied = min(n_bytes, MAX_FILENAME_SIZE);
    int final_byte = min(n_bytes, MAX_FILENAME_SIZE + 1);

    dentry_t dentry;
    if(read_dentry_by_index(file->fpos, &dentry) == 0){
        memcpy(buf, dentry.name, bytes_copied);
        ((uint8_t*)buf)[final_byte] = '\0';

        file->fpos++;
        return final_byte;
    }

    // No files are left, no bytes were read
    return 0;
}

/** dir_write
 * DESCRIPTION: Does nothing, file system in read-only
 * INPUTS: file - pointer to file in PCB
 *         buf - buffer to copy from
 *         n_bytes - number of bytes to read
 * RETURN VALUE: -1 on failure, 0 on success
 */
int32_t dir_write(file_t* file, const void* buf, int32_t n_bytes){
    return -1;
}

/** dir_open
 * DESCRIPTION: Does nothing
 * INPUTS:      file -- pointer to file_t (unused)
 * RETURN VALUE: 0 on success, -1 on failure
 */
int32_t dir_open(file_t* file){
    // Nothing to do here
    return 0;
}

/** dir_close
 * DESCRIPTION: Does nothing at the moment
 * INPUTS:      file -- pointer to file_t (unused)
 * RETURN VALUE: 0 on success, -1 on failure
 */
int32_t dir_close(file_t* file){
    return 0;
}

/** inode_at
 * DESCRIPTION: Helper function, returns pointer to inode at given index
 * INPUTS: index - index to retrieve inode from
 * OUTPUTS: inode_t pointer; NULL on failure
 */
inode_t* inode_at(uint32_t index){
    if(index > disk_img->n_inodes) return NULL;
    return (inode_t*)((int8_t*)disk_img + BLOCK_SIZE*(1 + index));
}

/** data_at
 * DESCRIPTION: Helper function, converts a given data block index into global address
 * INPUTS: index - index of data block
 * OUTPUTS: uint8_t* pointer to start of data block
 */
uint8_t* data_at(uint32_t index){
    // Convert data block index to global byte offset
    int32_t byte_offset = BLOCK_SIZE*(1 + disk_img->n_inodes + index);

    // Check for out of bouds
    if(byte_offset >= disk_size) return NULL;

    return (uint8_t*)disk_img + byte_offset;
}
