#include "pit.h"
#include "../lib/lib.h"
#include "i8259.h"
#include "../devices/terminal.h"

#define PIT_IRQ 0
#define PIT_BASE_FREQ 1193182
#define PIT_FREQ 1000

unsigned int counter = 0;
unsigned int one = 0;
unsigned int two = 0;
unsigned int three = 0;

void pit_init(void) {
  int divisor = PIT_BASE_FREQ / PIT_FREQ;
  outb(0x34, PIT_COMMAND);
  outb(divisor & 0xFF, PIT_CH0);
  outb(divisor >> 8, PIT_CH0);
  enable_irq(PIT_IRQ);
}

void increment_clock(void){
  uint32_t term = get_active_terminal();
  if(term == 0){
    one += 1;
    printf("one: %d\n", one);
  }
  else if(term == 1){
    two += 2;
    printf("two: %d\n", two);
  }
  else if(term == 2){
    three += 3;
    printf("three: %d\n", three);
  }
  else{
    printf("what\n");
  }
}
