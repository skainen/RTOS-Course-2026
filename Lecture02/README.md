| Scenario | Prediction before test | Actual observation | Did it match? Why? |
|---|---|---|---|
| A. Both tasks at priority 1 | Both execute | Both execute at the same time | |
| B. Task A priority 2; Task B priority 1 | B executes first | Both execute | No measurable difference |
| C. Task A priority 1; Task B priority 2 | A executes first | Both execute | No measurable difference |


Starvation:
Task A is printing repeatedly and LED is blinking normally. Task B has lower priority so it can only run when Task A is Blocked.
Since B kept running, A must have been blocking regularly even without vTaskDelay(). The only other thing in A's loop is Serial.println, which can wait for the output buffer to drain. So in this case the expected starvation didn't happen. If Task A never blocked, Task B would not get any CPU time.


Explanation:
On a single core only one task is Running at a time. Here each task takes microseconds.
vTaskDelay blocks because the task tells scheduler to not give it CPU time for X milliseconds.
The task is taken off the CPU and uses no processing time while its waiting for X time.
When the task delay expires it goes from Blocked to Ready, and it can run, but doesnt
necessarily run yet. Runs when its picked up by the scheduler.
Scheduler picks it by always running the highest priority Ready task. Equal priority
Ready tasks take turns, which is caled time slicing.
In my tests the priority didnt visibly change anything. Reason is probably
that both tasks were almost always in Blocked state. The higher priority task ran
first for microseconds which is not observable by just looking at it.
Removing the delay didnt change anything observable either except the serial print
was printing way faster. Blinking rate did not change visibly so something must have
been still blocking task A even though in theory it shouldnt when it has no delay and
is higher priority. Only thing in the loop was the Serial.println, so it was
waiting for the output buffer to clear and meanwhile the LED could blink.
