#include "spinlock.h"

#include "lib.h"

#define LOCKED 1
#define UNLOCKED 0

/** spin_lock_init
 * DESCRIPTION: initializes a spinlock
 * INPUTS: lock -- pointer to a spinlock
 */
void spin_lock_init(spinlock_t* lock){
    lock->lock = UNLOCKED;
}

/** spin_lock_irqsave
 * DESCRIPTION: locks spinlock, saves flags first
 * INPUTS: lock -- pointer to a spinlock
 * RETURN VALUE: flags
 */
unsigned long spin_lock_irqsave(spinlock_t* lock){
    unsigned long flags;

    // Save flags
    cli_and_save(flags);

    // Wait until unlocked
    while(lock->lock == LOCKED);
    
    // Lock it ourselves
    lock->lock = LOCKED;

    return flags;
}

/** spin_unlock_irqrestore
 * DESCRIPTION: unlocks spinlock, restores flags after
 * INPUTS: lock -- pointer to a spinlock
 */
void spin_unlock_irqrestore(spinlock_t* lock, unsigned long flags){
    // Unlock
    lock->lock = UNLOCKED;
    
    // Restore flags (including IF => no need to use sti)
    restore_flags(flags);
}
