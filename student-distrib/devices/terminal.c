#include "terminal.h"

#include "../lib/spinlock.h"
#include "../memory/paging.h"

/* Line buffer */
typedef struct {
    spinlock_t lock;
    uint32_t index;
    char buf[TERMINAL_BUF_SIZE];
    int reading;
    int terminal_x;
    int terminal_y;
} line_buf_t;

/* Multiple terminals */
static uint32_t active = 0;
static line_buf_t terminals[NUM_TERMINALS];

/* set_terminal
 * DESCRIPTION:         sets active terminal
 * INPUTS:              term -- new value
 */
void set_terminal(uint32_t term){
    active = term;
}

/* get_active_terminal
 * RETURNS:             the active terminal
 */
uint32_t get_active_terminal(){
    return active;
}

/** terminal_clear_screen
 * DESCRIPTION: Clears screen and reprints current terminal buffer
 * SIDE EFFECTS: changes screen content
 */
void terminal_clear_screen(){
    // This must always happen on the active terminal
    uint32_t t = get_active_terminal();
    pcb_t* pcb = get_pcb(active_pid);
    int old_x, old_y;
    if(pcb != NULL && t != pcb->terminal){
        set_lib_video_mem(-1);
        old_x = get_screen_x();
        old_y = get_screen_y();
        set_screen_pos(terminals[active].terminal_x, terminals[active].terminal_y);
    }

    clear();


    int i;
    for(i = 0; i < TERMINAL_BUF_SIZE; i++){
        if(terminals[active].buf[i] == '\0') break;
        putc(terminals[active].buf[i]);
    }

    if(pcb != NULL && t != pcb->terminal){
        // Go back to original vmem value
        set_terminal_pos(active, get_screen_x(), get_screen_y());
        set_screen_pos(old_x, old_y);
        set_lib_video_mem(pcb->terminal);
    }

    cursor_update();
}

/** terminal_init
 * DESCRIPTION:     initializes the terminal
 * SIDE EFFECTS:    clears line buffer, initializes spinlock
 */
void terminal_init(){
    for(active = 0; active < NUM_TERMINALS; active++){
        terminals[active].index = 0;
        terminals[active].reading = 0;
        memset(terminals[active].buf, '\0', TERMINAL_BUF_SIZE);
        spin_lock_init(&terminals[active].lock);
        init_video_mem(VIDEO_PTR(active));
    }

    set_terminal(0);
}

/* terminal_open
 * DESCRIPTION:     open handler for terminal, does nothing
 * INPUTS:          file -- pointer to file_t
 * RETURN VALUE:    0 on success, -1 on failure
 */
int32_t terminal_open(file_t* file){
    return 0;
}

/** terminal_fake_reading
 * DESCRIPTION: Sets terminal driver into "reading" mode without
 *              actually beginning to read. Useful for automated testing
 *              so that terminal_input doesn't clear on \n so that we can
 *              avoid needing to use interrupts
 * SIDE EFFECTS: sets "reading" flag
 */
void terminal_fake_reading(){
    unsigned long flags;
    flags = spin_lock_irqsave(&terminals[active].lock);
    terminals[active].reading = 1;
    spin_unlock_irqrestore(&terminals[active].lock, flags);
}

/** terminal_input
 * DESCRIPTION: handles raw input from the keyboard
 * INPUTS: c -- raw input character
 * RETURN VALUE: 0 on success, -1 if there was no space
 * SIDE EFFECTS: adds to line buffer
 */
int terminal_input(char c){
    unsigned long flags;
    int result = 0;
    int index = terminals[active].index;

    /* Aquire sole control over buffer */
    flags = spin_lock_irqsave(&terminals[active].lock);

    // If we don't have anyone reading, we can clear on \n
    if(c == '\n' && !terminals[active].reading){
        // Clear term_buffer
        index = 0;
        memset(terminals[active].buf, '\0', TERMINAL_BUF_SIZE);
    }
    // In canonical mode, process backspaces
    else if(c == '\b'){
        // Only if there is a key to erase
        if(index != 0){
            terminals[active].buf[--index] = '\0';
        }
        else{
            result = -1;
        }
    }
    // Add to buffer and increment index if there is space,
    //  force final character to be a newline
    else if((c == '\n' && index < TERMINAL_BUF_SIZE) || index < TERMINAL_BUF_SIZE - 1){
        terminals[active].buf[index++] = c;
    }
    else{
        // We couldn't process this byte
        result = -1;
    }

    // Save index changes
    terminals[active].index = index;

    // Print character if there was space in buffer
    if(result != -1){
        pcb_t* pcb = get_pcb(active_pid);
        if(pcb == NULL || pcb->terminal == active){
            putc(c);
            cursor_update();
        }
        else{
            set_lib_video_mem(-1);
            int old_x = get_screen_x();
            int old_y = get_screen_y();
            set_screen_pos(terminals[active].terminal_x, terminals[active].terminal_y);
            putc(c);
            set_terminal_pos(active, get_screen_x(), get_screen_y());
            set_lib_video_mem(pcb->terminal);
            set_screen_pos(old_x, old_y);
        }
    }

    /* Relenquish control over buffer */
    spin_unlock_irqrestore(&terminals[active].lock, flags);
    return result;
}

/** terminal_write
 * DESCRIPTION: write syscall handler for terminal.
 *              Writes supplied buffer to screen
 * INPUTS:  file -- pointer to file_t (unused)
 *          buf -- the buffer of bytes to write
 *          n -- the number of bytes to write
 * RETURN VALUE: number of bytes written or -1 on failure
 * SIDE EFFECTS: writes to the screen
 */
int32_t terminal_write(file_t* file, const void* buf, int32_t n){
    if(buf == NULL) return -1;

    int i;
    for(i = 0; i < n; i++){
        putc(((char*)buf)[i]);
    }

    cursor_update();
    return i;
}

/** terminal_read
 * DESCRIPTION: Reads line from terminal line-buffer
 *              (Wait until the next newline)
 * INPUTS:  file -- pointer to file_t (unused)
 *          buf -- the buffer to copy to
 *          n -- the number of bytes to read
 * RETURN VALUE: number of bytes read or -1 on failure
 */
int32_t terminal_read(file_t* file, void* buf, int32_t n){
    if(buf == NULL && n != 0) return -1;

    // Extra null check exists for tests which run outside of a task
    pcb_t* pcb = get_pcb(active_pid);
    uint32_t term = pcb == NULL ? 0 : pcb->terminal;

    // Wait for line to be done
    unsigned long flags;
    flags = spin_lock_irqsave(&terminals[term].lock);
    terminals[term].reading = 1;

    uint32_t index = terminals[term].index;
    while(index < TERMINAL_BUF_SIZE &&
         (index == 0 || (index > 0 && terminals[term].buf[index - 1] != '\n')))
    {
        spin_unlock_irqrestore(&terminals[term].lock, flags);
        flags = spin_lock_irqsave(&terminals[term].lock);
        index = terminals[term].index;
    }

    // Find how many bytes to read
    int n_bytes = min(n, index);
    memcpy((char*)buf, &terminals[term].buf, n_bytes);

    // Clear term_buffer
    terminals[term].index = 0;
    memset(terminals[term].buf, '\0', TERMINAL_BUF_SIZE);

    terminals[term].reading = 0;

    // Unlock buffer
    spin_unlock_irqrestore(&terminals[term].lock, flags);

    return n_bytes;
}

/** terminal_close
 * DESCRIPTION:     closes terminal driver
 * SIDE EFFECTS:    none
 * INPUTS:          file -- poitner to file_t (unused)
 * RETURN VALUE:    0 on success, -1 on failure
 */
int32_t terminal_close(file_t* file){
    // Nothing to do here
    return 0;
}

/* get_terminal_x
 * RETURNS:             the terminal_x of the specified terminal
 * INPUTS:              t -- the terminal
 */
int get_terminal_x(uint32_t t){
    if(t > NUM_TERMINALS) return -1;
    return terminals[t].terminal_x;
}

/* get_terminal_y
 * RETURNS:             the terminal_y of the specified terminal
 * INPUTS:              t -- the terminal
 */
int get_terminal_y(uint32_t t){
    if(t > NUM_TERMINALS) return -1;
    return terminals[t].terminal_y;
}

/* set_terminal_pos
 * DESCRIPTION:         sets terminal_x and terminal_y of the specified terminal
 * INPUTS:              t -- the terminal
 *                      x -- the new terminal_x
 *                      y -- the new terminal_y
 */
void set_terminal_pos(uint32_t t, int x, int y){
    if(t > NUM_TERMINALS) return;
    terminals[t].terminal_x = x;
    terminals[t].terminal_y = y;
}
