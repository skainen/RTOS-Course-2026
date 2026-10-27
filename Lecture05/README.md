## Purpose
The program demonstrates usage of a queue in RTOS.

## How It Works
Sensor Task simulates a temperature sensor's temperature increasing by 1. The value of the temperature sensor is copied into the queue.
The value in the queue is then copied from the queue in Display Task.

## Reflection
Queue is a safe way to pass data between tasks. Using a global variable can lead to race conditions, which is why we use a queue in this program.
