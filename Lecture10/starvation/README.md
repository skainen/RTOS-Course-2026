# Starvation

## Part A

1. Which tasks may experience starvation? Explain why.

    - Task_Log and Task_Display can starve. Task_Control has highest priority and never blocks.
    The scheduler always runs the highest-priority Ready task.

2. Why is it dangerous for a high-priority task to run continuously without blocking?

    - It takes the whole core. The lower-priority tasks can't make progress.

3. Give two ways to allow lower-priority tasks to get CPU time without changing task priorities.

    - Block periodically: Call vTaskDelay() or vTaskDelayUntil() in Task_Control to for example
    sample the sensor at a fixed rate. vTaskDelayUntil() gives a "jitter-free" period.

    Event-driven: Have Task_Control block on a semaphore, task notification or queue that is signalled by the sensor's data-ready ISR or hardware timer. Then it runs only when
    there is new data.


## Part B

### Step 1 - Observe starvation

```
t=27500 ms | Task_Hog: BUSY  | Task_Low progress: +0
t=28000 ms | Task_Hog: BUSY  | Task_Low progress: +0
t=28500 ms | Task_Hog: BUSY  | Task_Low progress: +0
t=29000 ms | Task_Hog: BUSY  | Task_Low progress: +0
t=29500 ms | Task_Hog: BUSY  | Task_Low progress: +0
t=30000 ms | Task_Hog: SLEEP | Task_Low progress: +0
t=30500 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=31000 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=31500 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=32000 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=32500 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=33000 ms | Task_Hog: SLEEP | Task_Low progress: +500
```

What is the progress of Task_Low while Task_Hog is BUSY?

    - Task_Low can't make progress.

What is the progress of Task_Low while Task_Hog is SLEEP?

    - Task_Low can progress.

What do these results tell you about starvation?

    - When Task_Hog is busy, the lower priority task can't make progress, and when it sleeps
    they can.


### Step 2 - Enable the fix


```
t=15500 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=16000 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=16500 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=17000 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=17500 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=18000 ms | Task_Hog: BUSY  | Task_Low progress: +490
t=18500 ms | Task_Hog: BUSY  | Task_Low progress: +46
t=19000 ms | Task_Hog: BUSY  | Task_Low progress: +45
t=19500 ms | Task_Hog: BUSY  | Task_Low progress: +46
t=20000 ms | Task_Hog: BUSY  | Task_Low progress: +45
t=20500 ms | Task_Hog: BUSY  | Task_Low progress: +46
t=21000 ms | Task_Hog: SLEEP | Task_Low progress: +54
t=21500 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=22000 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=22500 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=23000 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=23500 ms | Task_Hog: SLEEP | Task_Low progress: +500
```

Task_Low can now make progress because every 10 ms Task_Hog calls vTaskDelay(1) and enters the Blocked state until next tick. For that moment Task_Low is the highest-priority Ready task and it runs one step, then blocks on its own vTaskDelay(1). Both tasks wake up on the same next tick and Task_Hog wins again. Task_Low gets one step per 11m cycle (10ms busy + 1 tick blocked) which is 500 / 11 = 45 per 500 ms. No more starvation, but Task_Low still runs at only 9 % of its SLEEP rate.


### Step 3 - Change the delay

Prediction: Changing vTaskDelay(1) to 10 should give the lower priority task more time to progress.

```
t=12000 ms | Task_Hog: BUSY  | Task_Low progress: +490
t=12500 ms | Task_Hog: BUSY  | Task_Low progress: +250
t=13000 ms | Task_Hog: BUSY  | Task_Low progress: +250
t=13500 ms | Task_Hog: BUSY  | Task_Low progress: +250
t=14000 ms | Task_Hog: BUSY  | Task_Low progress: +250
t=14500 ms | Task_Hog: BUSY  | Task_Low progress: +250
t=15000 ms | Task_Hog: SLEEP | Task_Low progress: +260
t=15500 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=16000 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=16500 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=17000 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=17500 ms | Task_Hog: SLEEP | Task_Low progress: +500
t=18000 ms | Task_Hog: BUSY  | Task_Low progress: +490
t=18500 ms | Task_Hog: BUSY  | Task_Low progress: +250
t=19000 ms | Task_Hog: BUSY  | Task_Low progress: +250
t=19500 ms | Task_Hog: BUSY  | Task_Low progress: +250
t=20000 ms | Task_Hog: BUSY  | Task_Low progress: +250
t=20500 ms | Task_Hog: BUSY  | Task_Low progress: +250
```

Task_Hog now blocks for 10 ticks after every 10 ms of work. Task_Low gets one step per free tick so BUSY progress rises to half of the SLEEP rate.


At 1000 Hz, vTaskDelay(1) means "block until the next tick", which can be anywhere from 0 to 1 ms. Here it is close to a full 1 ms only because Task_Hog wakes on a tick boundary and then busy-waits a whole number of milliseconds. At a 100 Hz tick rate the same code would give different numbers. This is why pdMS_TO_TICKS() is the portable way to write delays.
