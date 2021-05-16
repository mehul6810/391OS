#ifndef _RTC_H
#define _RTC_H

#include "../lib/types.h"
#include "../interrupts/i8259.h"
#include "../storage/filesys.h"

/* Function to initialize the RTC, called in kernel.c */
void rtc_init(void);

/* Function to handle RTC interrupts */
void rtc_handler(void);

/* Function to reset rtc frequency to 2 Hz */
int32_t rtc_open(file_t* file);

/* Function to read the RTC interrupts, returns 0 only after an interrupt is encountered */
int32_t rtc_read(file_t* file, void* buf, int32_t nbytes);

/* Function to change the frequency of the RTC */
int32_t rtc_write(file_t* file, const void* buf, int32_t nbytes);

/* Function to close RTC (doesn't do anythign as we want RTC to run constantly) */
int32_t rtc_close(file_t* file);

#endif /* _RTC_H */
