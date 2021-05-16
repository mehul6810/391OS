#ifndef _SPINLOCK_H
#define _SPINLOCK_H

typedef struct{
    int lock;
} spinlock_t;

void spin_lock_init(spinlock_t* lock);
unsigned long spin_lock_irqsave(spinlock_t* lock);
void spin_unlock_irqrestore(spinlock_t* lock, unsigned long flags);

#endif /* _SPINLOCK_H */
