# Lecture 9 - Assignment - Timer Events in a Queue

## Hardware

ESP32-S3, 220ohm resistor and an LED connected to pin 4.

## Analysis

A timer interrupt happens every 100 ms. Each interrupt gets a number (1, 2, 3, ...) and tries to put it in a queue that holds 4 numbers. If the queue is full, that number is thrown away, but the next interrupt still gets the next number.
A task takes numbers from the queue, prints them, and then waits before taking the next one.

# Prediction for run A (task waits 20 ms)

All numbers are printed in order. The task is faster than the interrupts, so the queue never fills up.

# Prediction for run B (task waits 500 ms)

The queue will fill up. Ten numbers come in every second, but the task takes only two. After about half a second the queue is full and numbers start being thrown away.

## Explanation

When the queue is full, the new number is thrown away and the numbers already
in the queue stay there. So when 7 to 10 are thrown away, 3 to 6 are still
waiting in the queue. The task prints those first, one every half second.
That is why the gap shows up in the output about 2 seconds after the numbers were actually lost.

After that, only one number in five fits into the queue so the output jumps by 5 each time (11, 16, 21, ...).
