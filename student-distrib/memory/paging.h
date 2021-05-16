#ifndef _PAGING_H
#define _PAGING_H

#define PDE_SIZE    4
#define PD_LENGTH   1024
#define PD_SIZE     (PD_LENGTH * PDE_SIZE)

#define PTE_SIZE    4
#define PT_LENGTH   1024
#define PT_SIZE     (PT_LENGTH * PTE_SIZE)

#define KERNEL_BASE_ADDR    0x00400000

#define VIDEO_BASE_ADDR     0x000B8000
#define VIDEO_PT_OFFSET     ((0x000B8000 & 0x003FF000) >> 12)
#define VIDEO_SIZE          0x1000
#define VIDEO_PTR(t)        ((uint8_t*)(VIDEO_BASE_ADDR + (t+1)*VIDEO_SIZE))

/* Constants defined in terms of physical address space */
#define USER_PDE            32
#define USER_BASE_ADDR      0x00800000
#define USER_BASE_OFFSET    0x00400000

/* Constants defined in terms of virtual addresses space */
#define USER_PAGE_START     0x08000000
#define USER_LOAD_ADDR      0x08048000
#define USER_STACK          0x08400000
#define USER_KTASK_BASE     0x00800000
#define USER_KTASK_OFFSET   0x00002000

#define USER_VIDEO_PDE      33
#define USER_VIDEO_ADDR     (USER_VIDEO_PDE * 0x00400000)

/* Page directory entry struct */
typedef struct {
    unsigned int p :        1;
    unsigned int rw :       1;
    unsigned int us :       1;
    unsigned int pwt :      1;
    unsigned int pcd :      1;
    unsigned int a :        1;
    unsigned int dirty :    1;
    unsigned int ps :       1;
    unsigned int g :        1;
    unsigned int avail :    3;
    unsigned int base :     20;

} PDE_t;

/* Page table entry struct */
typedef struct {
    unsigned int p :        1;
    unsigned int rw :       1;
    unsigned int us :       1;
    unsigned int pwt :      1;
    unsigned int pcd :      1;
    unsigned int a :        1;
    unsigned int dirty :    1;
    unsigned int pat :      1;
    unsigned int g :        1;
    unsigned int avail :    3;
    unsigned int base :     20;

} PTE_t;

#include "../tasks/process.h"

/* initialize paging function defined in paging.c */
void init_paging(void);

/* load page directory and set cr0-3-4 registers defined in pagingassembly.c */
extern void load_pd(PDE_t* ptr);
extern void flush_tlb(void);

/* create a page for the task */
void setup_task_page(int pid);

/* Create vid page for user tasks */
void setup_user_video_mem(pcb_t* pcb);
void disable_user_video_mem();

/* delete last task page */
void delete_task_page();

#endif
