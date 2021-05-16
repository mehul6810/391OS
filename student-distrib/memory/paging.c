#include "paging.h"

static PDE_t page_dir[PD_LENGTH] __attribute__((aligned (4096)));
static PTE_t page_table[PT_LENGTH] __attribute__((aligned (4096)));
static PTE_t page_table_vidmap[PT_LENGTH] __attribute__((aligned (4096)));

/* init_paging
 * DESCRIPTION: Initializes page directory and page table of the first PDE
                Sets 0-4MB page to 4KB pages and makes 0x0b8000 present for video
                Sets 4-8MB page to present for kernel
 * OUTPUTS: none
 * SIDE EFFECTS: sets bits in page_directory and page_table accordingly to instructions
 */
void init_paging() {
    int i;

    /* Set all page table entries to unmapped (initially) */
    for (i = 0; i < PT_LENGTH; ++i) {
        page_table[i].p = 0;
    }

    /* Map video video memory directly to physical memory */
    page_table[VIDEO_PT_OFFSET].base = VIDEO_BASE_ADDR >> 12;
    page_table[VIDEO_PT_OFFSET].pat = 0;
    page_table[VIDEO_PT_OFFSET].dirty = 0;
    page_table[VIDEO_PT_OFFSET].pcd = 0;
    page_table[VIDEO_PT_OFFSET].pwt = 0;
    page_table[VIDEO_PT_OFFSET].us = 0;
    page_table[VIDEO_PT_OFFSET].rw = 1;
    page_table[VIDEO_PT_OFFSET].p = 1;

    // Map terminal vmem buffers next to actual video memory
    int t;
    for(t = 1; t < NUM_TERMINALS + 1; t++){
        page_table[VIDEO_PT_OFFSET + t].base = (VIDEO_BASE_ADDR >> 12) + t * VIDEO_SIZE;
        page_table[VIDEO_PT_OFFSET + t].pat = 0;
        page_table[VIDEO_PT_OFFSET + t].dirty = 0;
        page_table[VIDEO_PT_OFFSET + t].pcd = 0;
        page_table[VIDEO_PT_OFFSET + t].pwt = 0;
        page_table[VIDEO_PT_OFFSET + t].us = 0;
        page_table[VIDEO_PT_OFFSET + t].rw = 1;
        page_table[VIDEO_PT_OFFSET + t].p = 1;
    }

    // /* Map 0 - 4MB of virtual memory to 4kB page table entries */
    page_dir[0].base = (int) page_table >> 12;
    page_dir[0].ps = 0;
    page_dir[0].dirty = 0;
    page_dir[0].pcd = 1;
    page_dir[0].pwt = 0;
    page_dir[0].us = 0;
    page_dir[0].rw = 1;
    page_dir[0].p = 1;

    /* Map 4Mb - 8Mb of virtual memory directly to physical memory for kernel */
    page_dir[1].base = KERNEL_BASE_ADDR >> 12;
    page_dir[1].g = 1;
    page_dir[1].ps = 1;
    page_dir[1].dirty = 0;
    page_dir[1].pcd = 1;
    page_dir[1].pwt = 0;
    page_dir[1].us = 0;
    page_dir[1].rw = 1;
    page_dir[1].p = 1;

    /* Set rest of page directory to unmapped */
    for (i = 2; i < PD_LENGTH; ++i) {
        page_dir[i].p = 0;
    }

    load_pd(page_dir);
}

/* setup_task_page
 * DESCRIPTION:     Initializes a page in the directory for a user task
 *                  Maps virtual address 128MB to appropriate physical address
 * INPUTS:          pid -- the process id of the task
 * SIDE EFFECTS:    Sets bits in page_directory accordingly to a user task
 */
void setup_task_page(int pid){
  page_dir[USER_PDE].p = 1;
  page_dir[USER_PDE].rw = 1;
  page_dir[USER_PDE].us = 1;
  page_dir[USER_PDE].pwt = 0;
  page_dir[USER_PDE].pcd = 1;
  page_dir[USER_PDE].dirty = 0;
  page_dir[USER_PDE].ps = 1;
  page_dir[USER_PDE].g = 0;
  page_dir[USER_PDE].base = (USER_BASE_ADDR + pid*USER_BASE_OFFSET) >> 12;

  // Reset TLB
  flush_tlb();
}

/* setup_user_video_mem
 * DESCRIPTION:     Initializes user page to point to video memory
 * INPUTS:          pcb -- the pointer to the PCB of the process that will
 *                      be using this video memory pointer
 * SIDE EFFECTS:    Changes page directory
 */
void setup_user_video_mem(pcb_t* pcb){
  if(pcb == NULL) return;
  if(!(pcb->flags & TASK_VID_IN_USE)) return;

  uint32_t video_base;

  if(pcb->terminal == get_active_terminal()){
      video_base = VIDEO_BASE_ADDR;
  }
  else{
      video_base = (uint32_t)VIDEO_PTR(pcb->terminal);
  }

  /* Map video memory directly to physical memory */
  page_table_vidmap[0].base = video_base >> 12;
  page_table_vidmap[0].pat = 0;
  page_table_vidmap[0].dirty = 0;
  page_table_vidmap[0].pcd = 0;
  page_table_vidmap[0].pwt = 0;
  page_table_vidmap[0].us = 1;
  page_table_vidmap[0].rw = 1;
  page_table_vidmap[0].p = 1;

  /* Map virtual memory to 4kB page table entries */
  page_dir[USER_VIDEO_PDE].base = (int) page_table_vidmap >> 12;
  page_dir[USER_VIDEO_PDE].ps = 0;
  page_dir[USER_VIDEO_PDE].dirty = 0;
  page_dir[USER_VIDEO_PDE].pcd = 1;
  page_dir[USER_VIDEO_PDE].pwt = 0;
  page_dir[USER_VIDEO_PDE].us = 1;
  page_dir[USER_VIDEO_PDE].rw = 1;
  page_dir[USER_VIDEO_PDE].p = 1;

  flush_tlb();
}

/* disable_user_video_mem
 * DESCRIPTION:     Deletes user page to point to video memory
 * SIDE EFFECTS:    Changes page directory
 */
void disable_user_video_mem(){
    page_dir[USER_VIDEO_PDE].p = 0;
    page_table_vidmap[0].p = 0;

    flush_tlb();
}

/* delete_task_page
 * DESCRIPTION:     Set's user page to not present
 * SIDE EFFECTS:    Sets bits in page_directory accordingly to a user task
 */
void delete_task_page(){
  page_dir[USER_PDE].p = 0;

  // Reset TLB
  flush_tlb();
}
