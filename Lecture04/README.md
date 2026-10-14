a. Yes. The baseline was same, but each task creation consumed more heap.

b. Free heap dropped by stack size + 376 bytes per task in both runs. The 376 bytes is task control block (TCB) + heap allocator bookkeeping. Doubling the stack from 4096 to 8192 increased each task's cost by 4096 bytes.

c. The stack holds a task's execution state including local variables, function call return addresses, saved registers, and arguments passed between nested calls. Each task needs its own stack because tasks are preempted independently. When the scheduler switches away from a task, CPU registers are saved onto its own stack so it can resume exactly where it left off. A too-small stack overflows into adjacent memory.

d. Creating a task means the kernel needs to reserve memory that belongs to that task as long as it exists. It needs the TCB, where the scheduler tracks the task's priority, state, name, and saved stack pointer. It also needs a private stack sized to whatever was requested. xTaskCreate allocates both dynamically from the heap, so free heap drops by roughly the stack size plus the fixed overhead consisting of TCB and bookkeeping.
