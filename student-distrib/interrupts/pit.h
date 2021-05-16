#define PIT_CH0 0x40
#define PIT_COMMAND 0x43
#define PIT_IRQ 0

void pit_init(void);
void increment_clock(void);
