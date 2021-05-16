#include "syscalls.h"

#include "../memory/paging.h"
#include "../tasks/process.h"
#include "../storage/filesys.h"
#include "../devices/rtc.h"
#include "../arch/x86_desc.h"
#include "../devices/terminal.h"

#define ELF_MAGIC_LEN 4
#define ELF_MAGIC {0x7f, 0x45, 0x4C, 0x46};
#define ELF_ENTRYPT_OFFSET 24

// Global exception flag for do_halt
volatile int32_t exception_flag = 0;

// There are 3 other types of fops tables
static struct file_ops file_fops = {
    .open = file_open,
    .close = file_close,
    .read = file_read,
    .write = file_write
};
static struct file_ops dir_fops = {
    .open = dir_open,
    .close = dir_close,
    .read = dir_read,
    .write = dir_write
};
static struct file_ops rtc_fops = {
    .open = rtc_open,
    .close = rtc_close,
    .read = rtc_read,
    .write = rtc_write
};

/* syscall_fail
 * DESCRIPTION:     Used for invalid fops entries
 * RETURNS:         -1
 */
int32_t syscall_fail(){
    return -1;
}

/* do_open
 * DESCRIPTION:     Executes open syscall
 * INPUTS:          filename -- file name to search for and open
 * RETURNS:         0 on success, -1 on failure
 * SIDE EFFECTS:    adds to the files array of the calling process
 */
int32_t do_open (const uint8_t* filename){
    if(filename == NULL) return -1;

    pcb_t* pcb = get_pcb(active_pid);

    int err; // Used to store error values of helper functions

    // Get file pointer and mark as open
    int fd = 0;
    while(pcb->files[fd].flags == FILE_IN_USE){
        if(++fd >= MAX_FILES) return -1;
    }
    file_t* file = &pcb->files[fd];

    dentry_t dentry;
    err = read_dentry_by_name((int8_t*)filename, &dentry);
    if(err != 0) return err;

    // Check type of file and set file ops
    switch(dentry.type){
        case DENTRY_TYPE_RTC:
            file->ops = &rtc_fops;
            break;
        case DENTRY_TYPE_DIR:
            file->ops = &dir_fops;
            break;
        case DENTRY_TYPE_FILE:
            file->ops = &file_fops;
            break;
        default:
            // We shouldn't ever get here
            return -1;
    }

    file->inode = dentry.inode;
    file->fpos = 0;
    file->flags = FILE_IN_USE;

    // Call file initializer
    if(file->ops->open(file) != 0){
        file->flags = 0;
        return -1;
    }

    return fd;
}

/* do_close
 * DESCRIPTION:     the close syscall handler
 * INPUTS:          fd -- the file descriptor
 * RETURNS:         0 on success, -1 on failure
 * SIDE EFFECTS:    changes the files array for this process
 */
int32_t do_close (int32_t fd){
    // Check that file descriptor is valid
    if(fd >= MAX_FILES || fd < 0) return -1;

    // stdin/stdout are not closable
    if(fd == STDIN || fd == STDOUT) return -1;

    pcb_t* pcb = get_pcb(active_pid);
    file_t* file = &pcb->files[fd];

    // If file is already closed, report error
    if(file->flags != FILE_IN_USE) return -1;

    // If file can't be closed, don't close it!
    int err;
    err = file->ops->close(file);
    if(err != 0){
        return err;
    }

    // Unset flags to allow a new file to be opened with this file descriptor
    file->flags = 0;
    return 0;
}

/* do_read
 * DESCRIPTION:     the read syscall handler
 * INPUTS:          fd -- the file descriptor
 *                  buf -- the buffer to read into
 *                  nbytes -- the number of bytes to read
 * RETURNS:         the number of bytes read or -1 on failure
 */
int32_t do_read (int32_t fd, void* buf, int32_t nbytes){
    // Check that file descriptor is valid
    if(fd >= MAX_FILES || fd < 0 || fd == STDOUT) return -1;
    // Check that buffer is valid
    if(buf == NULL) return -1;

    pcb_t* pcb = get_pcb(active_pid);
    file_t* file = &pcb->files[fd];

    // Check that file is in use
    if(file->flags != FILE_IN_USE) return -1;

    return file->ops->read(file, buf, nbytes);
}

/* do_write
 * DESCRIPTION:     the write syscall handler
 * INPUTS:          fd -- the file descriptor
 *                  buf -- the buffer to write from
 *                  nbytes -- the number of bytes to write
 * RETURNS:         the number of bytes written or -1 on failure
 */
int32_t do_write (int32_t fd, const void* buf, int32_t nbytes){
    // Check that file descriptor is valid
    if(fd >= MAX_FILES || fd < 0 || fd == STDIN) return -1;
    // Check that buffer is valid
    if(buf == NULL) return -1;

    pcb_t* pcb = get_pcb(active_pid);
    file_t* file = &pcb->files[fd];

    // Check that file is in use
    if(file->flags != FILE_IN_USE) return -1;

    return file->ops->write(file, buf, nbytes);
}

/* do_halt
 * DESCRIPTION:     the halt syscall handler
 * INPUTS:          status -- the exit status
 * RETURNS:         0 on success, -1 on failure
 */
int32_t do_halt (uint8_t status){
    /* Must not be interrupted by scheduler */
    cli();

    // Check if program halted due to exception
    int32_t retval;
    if (exception_flag != 0) {
        retval = EXCEPTION_STATUS;
        exception_flag = 0;
    } else{
        retval = (int32_t) status;
    }

    // Close all files (ignore stdin/stdout)
    int fd;
    for(fd = 2; fd < MAX_FILES; fd++){
        do_close(fd);
    }

    pcb_t* pcb = get_pcb(active_pid);

    if(pcb->flags & TASK_VID_IN_USE){
        disable_user_video_mem();
    }

    // Restore parent state
    pid_t parent = pcb->parent_pid;
    set_terminal_pid_head(pcb->terminal, parent);
    free_pid(active_pid);

    // Invalid parent PID implies a shell must be respawned for this terminal
    if(parent <= MAX_PID){
        setup_task_page(parent);
        tss.esp0 = get_kernel_stack(parent);
        active_pid = parent;
    }
    else{
        // A shell must always be running, respawn shell
        active_pid = -1;
        do_execute((uint8_t*)"shell");
    }

    // Return to parent task
    pcb_t* parent_pcb = get_pcb(parent);
    parent_pcb->flags &= ~(TASK_WAITING_FOR_CHILD);
    parent_pcb->flags |= TASK_EXECUTING;
    asm volatile (
        "movl %0, %%esp;"
        "movl %1, %%ebp;"
        "movl %2, %%eax;"
        "leave;"
        "ret;"
        : /* no output */
        : "r"(parent_pcb->context.esp), "r"(parent_pcb->context.ebp), "r"(retval)
        : "esp", "ebp", "eax"
    );

    // Should never get here
    return -1;
}

/* prep_task
 * DESCRIPTION:     helper for do_execute which preps a task but does not start it
 * INPUTS:          command -- pointer to command name
 * RETURNS:         -1 on failure, pid of task on success
 */
pid_t prep_task(const uint8_t* command){
    if(command == NULL) return -1;
    if(num_tasks >= MAX_TASKS) return -1;

    // Parse args
    char p_name[MAX_FILENAME_SIZE];
    char args[TERMINAL_BUF_SIZE];
    memset(p_name, '\0', MAX_FILENAME_SIZE);
    memset(args, '\0', TERMINAL_BUF_SIZE);
    int i = 0, offset = 0;

    // Skip leading spaces
    while(command[i] == ' '){i++;}

    // Save program name
    offset = i;
    while(command[i] != ' ' && command[i] != '\0' && i < MAX_FILENAME_SIZE){
        p_name[i - offset] = command[i];
        i++;
    }

    // Skip spaces in between
    while(command[i] == ' '){i++;}

    // Save program args
    offset = i;
    while(command[i] != ' ' && command[i] != '\0' && i < TERMINAL_BUF_SIZE){
        args[i - offset] = command[i];
        i++;
    }

    // Get file info
    dentry_t dentry;
    if(read_dentry_by_name(p_name, &dentry) != 0){
        // Requested executable doesn't exist
        return -1;
    }
    inode_t* inode = inode_at(dentry.inode);

    // Check for executable magic
    int8_t magic[ELF_MAGIC_LEN];
    int8_t expected_magic[ELF_MAGIC_LEN] = ELF_MAGIC;
    read_data(dentry.inode, 0, (uint8_t*)magic, ELF_MAGIC_LEN);
    if(strncmp(magic, expected_magic, ELF_MAGIC_LEN) != 0){
        // Specified file is not an executable
        return -1;
    }

    // Command is runnable, begin process
    pid_t pid = reserve_pid();
    if(pid > MAX_PID) return -1;

    // Special case for initial shells
    pcb_t* parent_pcb;
    if(active_pid > MAX_PID){
        // Invalid active_pid indicates that active terminal is empty
        uint32_t terminal = get_active_terminal();

        parent_pcb = NULL;
        set_terminal_pid_head(terminal, pid);
    }
    else{
        parent_pcb = get_pcb(active_pid);
        set_terminal_pid_head(parent_pcb->terminal, pid);
        parent_pcb->flags &= ~(TASK_EXECUTING);
        parent_pcb->flags |= TASK_WAITING_FOR_CHILD;
    }

    // Load file to memory
    setup_task_page(pid);
    read_data(dentry.inode, 0, (uint8_t*)USER_LOAD_ADDR, inode->size);

    // Create PCB
    pcb_t* pcb = get_pcb(pid);
    init_pcb(pcb, pid, args);

    // Save entry info
    read_data(dentry.inode, ELF_ENTRYPT_OFFSET, (uint8_t*)&(pcb->context.eip), 4);

    // Simulate interrupt context before calling resume_task
    pcb->context.esp = get_kernel_stack(pid);
    uint32_t old_stack, temp_flags;

    // Preprocessor stuff for stringizing constants
    #define STR(val) _STR(val)
    #define _STR(val) #val
    #define USER_CS_STR STR(USER_CS)
    #define USER_DS_STR STR(USER_DS)
    #define USER_STACK_STR STR(USER_STACK)

    asm volatile(
        "movl %%esp, %0;"
        "movl %2, %%esp;"

        // Fake interrupt context
        "pushl $"USER_DS_STR";"
        "pushl $"USER_STACK_STR";"
        "pushfl;"
        "popl %1;"
        "orl $0x200, %1;" // Ensure interrupts are enabled
        "pushl %1;"
        "pushl $"USER_CS_STR";"
        "pushl %3;"

        "pushl $0;"                     // Fake error code
        "pushal;"
        "movl %0, %%esp;"            // Return to this function
        : "=&r"(old_stack), "=&r"(temp_flags)
        : "r"(pcb->context.esp), "r"(pcb->context.eip)
        : "memory"
    );

    // resume_task assumes ESP points between interrupt context and register state
    pcb->context.esp -= 24;

    return pid;
}

/* do_execute
 * DESCRIPTION:     the execute syscall handler
 * INPUTS:          command -- pointer to command name
 * RETURNS:         0 on success, 1 on failure due to too many tasks,
 *                  -1 on other failures
 */
int32_t do_execute (const uint8_t* command){
    if(num_tasks >= MAX_TASKS) return 1;

    cli();

    pid_t pid = prep_task(command);
    if(pid > MAX_PID) return -1;

    pcb_t* parent_pcb = active_pid > MAX_PID ? NULL : get_pcb(active_pid);

    // Save parent's stack registers for use in halt
    if(parent_pcb != NULL){
        asm volatile(
            "movl %%esp, %0;"
            "movl %%ebp, %1;"
            : "=r"(parent_pcb->context.esp), "=r"(parent_pcb->context.ebp)
            : /* no input */
        );

        // Save terminal position
        set_terminal_pos(parent_pcb->terminal, get_screen_x(), get_screen_y());
    }
    else{
        // Find the containing terminal and save terminal position
        uint32_t t;
        for(t = 0; t < NUM_TERMINALS; t++){
            // Terminal PID head was set in prep_task
            if(get_terminal_pid_head(t) == pid) break;
        }
        set_terminal_pos(t, get_screen_x(), get_screen_y());
    }

    resume_task(pid);

    // Should never reach here, will be exited by halt
    return -1;
}

/* do_getargs
 * DESCRIPTION:     the getargs syscall handler
 * INPUTS:          buf -- the buffer to write into
 *                  nbytes -- the number of bytes available in buf
 * RETURNS:         0 on success or -1 on failure
 */
int32_t do_getargs (uint8_t* buf, int32_t nbytes){
    if(buf == NULL || nbytes < 0) return -1;

    pcb_t* pcb = get_pcb(active_pid);
    int bytes_read = min(nbytes, TERMINAL_BUF_SIZE);
    memcpy(buf, pcb->args, bytes_read);
    if(buf[0] == '\0' || buf[32] != '\0') return -1;
    return 0;
}

/* do_vidmap
 * DESCRIPTION:     the vidmap syscall handler
 * INPUTS:          screen_start -- pointer to a pointer which user wants to
 *                                  point at video memory
 * RETURNS:         0 on success, -1 on failure
 */
int32_t do_vidmap (uint8_t** screen_start){
    if(screen_start == NULL) return -1;

    // Confirm that pointer is within 4MB user page
    if(screen_start < (uint8_t**) USER_PAGE_START
    || screen_start > (uint8_t**) USER_PAGE_START + USER_BASE_OFFSET)
        return -1;

    // Set up user-mode video memory page
    pcb_t* pcb = get_pcb(active_pid);
    pcb->flags |= TASK_VID_IN_USE;
    setup_user_video_mem(pcb);
    *screen_start = (uint8_t*)USER_VIDEO_ADDR;

    return 0;
}

/* do_set_handler
 * DESCRIPTION:     the set_handler syscall handler
 * INPUTS:          signum -- the signal to register this handler to
 *                  handler -- pointer to the handler function
 * RETURNS:         0 on success, -1 on failure
 */
int32_t do_set_handler (int32_t signum, void* handler){
    return 0;
}

/* do_sigreturn
 * DESCRIPTION:     the sigreturn syscall handler
 * RETURNS:         0 on success, -1 on failure
 */
int32_t do_sigreturn (void){
    return 0;
}
