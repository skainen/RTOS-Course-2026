## Lecture 7 - Semaphore

`emptySlots` is a counting semaphore (max 5, initial 5) that counts free slots in the buffer.

`filledSlots` (max 5, initial 0) counts slots holding a value.
They start at 5 and 0 because the buffer is initially empty so all slots are free.

`bufferMutex` = mutual exclusion. Only one task at a time can access the shared buffer, indices and counters. A task trying to take it while another task holds it blocks until it is given back.

A producer waits at `xSemaphoreTake(emptySlots)` when the buffer is full.
A consumer waits at `xSemaphoreTake(filledSlots)` when the buffer is empty.
Neither holds the mutex while waiting. Taking the mutex first would deadlock: a producer holding it on a full buffer would prevent consumers from freeing a slot.

In the second test I changed CONSUMER_DELAY_MS value to 2000ms.
All 15 values were consumed and each ID 0–4 appeared 3 times. The read order differed from the simple 0,1,2,3,4 pattern (e.g. 2,3,4,0,1 in later rounds) because the order
in which blocked producers obtain free slots varies.