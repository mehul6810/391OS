# void flush_tlb()
# DESCRIPTION:      flushes tlb
.globl flush_tlb
flush_tlb:
    movl %cr3, %eax
    movl %eax, %cr3
    ret

# void load_pd(PDE_t* ptr)
# load page_directory into cr3 to allow paging
# Input			: ptr - pointer to page directory
# Output		: nothing
# Side effect: Sets bit 31 and 0 of cr0 to 1 to enable paging on the machine
#              Sets bit 1 of cr4 to 1 to enable mixed page sizes
#              Sends address of page directory into cr3
.globl load_pd
load_pd:
  # callee convention
  pushl %ebp
  movl %esp, %ebp
  pushl %esi
  pushl %edi
  pushl %ebx

  movl 8(%ebp), %esi        # move page directory address into esi
  movl %esi, %cr3           # move page directory address into cr3

  movl  %cr4, %esi          # fetch cr4 register contents
  orl   $0x00000010, %esi   # set the PSE bit to allow mixed page sizes
  movl  %esi, %cr4          # write back to cr4

  movl  %cr0, %esi          # fetch cr0 register contents
  orl   $0x80000001, %esi   # Set the PE and PG bits in cr0
  movl  %esi, %cr0          # write back to cr0 register

  # callee cleanup
  popl %ebx
  popl %edi
  popl %esi
  leave
  ret

set_paging:
  # callee convention
  pushl %ebp
  movl %esp, %ebp
  pushl %esi
  pushl %edi
  pushl %ebx
  # or $0x80000000, %cr0 # set paging flag to 1 in cr0
  # callee cleanup
  popl %ebx
  popl %edi
  popl %esi
  leave
  ret
