#ifndef _TERMINAL_H
#define _TERMINAL_H

#define TERMINAL_BUF_SIZE 128
#define NUM_TERMINALS 3

#include "../lib/types.h"
#include "../storage/filesys.h"
#include "../tasks/process.h"

/* Functions */
void set_terminal(uint32_t term);
uint32_t get_active_terminal();

int get_terminal_x(uint32_t t);
int get_terminal_y(uint32_t t);
void set_terminal_pos(uint32_t t, int x, int y);

int terminal_input(char c);
void terminal_fake_reading(void);
void terminal_clear_screen(void);
void terminal_init(void);

int32_t terminal_open(file_t* file);
int32_t terminal_write(file_t* file, const void* buf, int32_t n);
int32_t terminal_read(file_t* file, void* buf, int32_t n);
int32_t terminal_close(file_t* file);

#endif /* _TERMINAL_H */
