# Deadlock assignment

## Part A

1. Which two tasks can cause deadlock? Explain how they can end up waiting for each other.

    - Task_Sensor and Task_Logger. How? Task_Sensor takes i2cMutex and before it gets uartMutex, the scheduler switches to Task_Logger, which takes uartMutex. Each task holds the mutex the other one needs and blocks waiting for it.

2. Can Task_Display create a circular wait by itself? Explain why or why not.

    - No. It only holds the i2cMutex. It doesn't wait for another mutex while holding. Though, it can be blocked if the other two tasks are in deadlock.

3. Which Coffman condition is removed when all tasks acquire mutexes in the same order?

    - Circular wait.


## Part B

### Step 1

```
[Task_1] taking A ...
[Task_1] has A, now needs B
[Task_2] taking B ...
[Task_2] has B, now needs A
[HEARTBEAT] t=2 s | rounds: Task_1=0 Task_2=0 Task_3=0
[HEARTBEAT] t=3 s | rounds: Task_1=0 Task_2=0 Task_3=0
[HEARTBEAT] t=4 s | rounds: Task_1=0 Task_2=0 Task_3=0
[HEARTBEAT] t=5 s | rounds: Task_1=0 Task_2=0 Task_3=0
```

What happens to the worker tasks?

    - Task 1 holds A and waits for B, while Task 2 holds B and waits for A. Both wait with portMAX_DELAY. They are in deadlock.


Which numbers in the HEARTBEAT line stop changing?

    - The task counter lines.

### Step 2

```
[Task_1] taking A ...
[Task_2] has BOTH locks -> work done (round 276)
[Task_3] got B after waiting 50 ms (round 553)
[Task_1] has A, now needs B
[Task_2] taking A ...
[Task_1] has BOTH locks -> work done (round 277)
[Task_3] got B after waiting 50 ms (round 554)
[Task_2] has A, now needs B
[HEARTBEAT] t=84 s | rounds: Task_1=277 Task_2=276 Task_3=554
```

The tasks can continue to work because they take the mutex in same order. Circular wait can't happen.


### Step 3

When a task can't get its second mutex within 500 ms, xSemaphoreTake returns pdFALSE. The task prints TIMEOUT, releases the mutex it holds and sleeps for a random 100-400 ms before trying again. It has to release the first mutex because otherwise the cycle is never broken. The other task stays blocked on that mutex and releasing it is what lets the other task get both locks and finish. The random backoff makes the two tasks retry at different times so they don't keep taking their first locks at the same time and timing out together over and over (livelock).