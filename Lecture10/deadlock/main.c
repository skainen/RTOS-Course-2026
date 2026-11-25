/*
  Real Time Systems (4_EFS8050) - Lecture 10
  Demo 1 - Deadlock

  OBJECTIVE
    Show how two tasks get stuck forever when they take two mutexes in
    opposite order, and how a consistent lock order (or a timeout) fixes it.
    Task_3 uses only mutex B, so B is a genuinely shared resource in every mode.

  HARDWARE
    ESP32-S3 DevKitC-1 + USB cable. No other parts.

  HOW TO RUN
    Board: "ESP32S3 Dev Module". Serial Monitor: 115200 baud.
    If you use the board's native USB port, set Tools > USB CDC On Boot > Enabled.
    Change DEMO_MODE below, upload, press RESET, watch the Serial Monitor.

  MODES
    0 = UNSAFE   : opposite lock order, wait forever   (INTENTIONAL deadlock)
    1 = TIMEOUT  : opposite lock order, finite timeout + random backoff
    2 = SAFE     : same lock order for all tasks (A before B)  <- recommended

  NOTE: Mode 0 is a teaching example. Never write production code like this.
*/
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define DEMO_MODE 1

SemaphoreHandle_t mutexA;
SemaphoreHandle_t mutexB;

struct LockPlan {
  const char*       taskName;
  SemaphoreHandle_t first;
  const char*       firstName;
  SemaphoreHandle_t second;      // NULL = single-lock task
  const char*       secondName;
  volatile uint32_t rounds;
};

LockPlan plan1;
LockPlan plan2;
LockPlan plan3;

void initPlan(LockPlan &p, const char* name,
              SemaphoreHandle_t f, const char* fName,
              SemaphoreHandle_t s, const char* sName) {
  p.taskName = name;
  p.first = f;   p.firstName = fName;
  p.second = s;  p.secondName = sName;
  p.rounds = 0;
}

// Two-lock worker (Task_1, Task_2)
void workerTask(void* pv) {
  LockPlan* p = (LockPlan*)pv;
  for (;;) {
    Serial.printf("[%s] taking %s ...\n", p->taskName, p->firstName);
    xSemaphoreTake(p->first, portMAX_DELAY);
    Serial.printf("[%s] has %s, now needs %s\n", p->taskName, p->firstName, p->secondName);

    vTaskDelay(pdMS_TO_TICKS(100));   // simulated work: makes the bad timing certain

#if DEMO_MODE == 1
    TickType_t waitTime = pdMS_TO_TICKS(500);   // finite timeout
#else
    TickType_t waitTime = portMAX_DELAY;        // wait forever
#endif

    if (xSemaphoreTake(p->second, waitTime) == pdTRUE) {
      p->rounds++;
      Serial.printf("[%s] has BOTH locks -> work done (round %lu)\n",
                    p->taskName, (unsigned long)p->rounds);
      vTaskDelay(pdMS_TO_TICKS(50));
      xSemaphoreGive(p->second);      // release in reverse order
      xSemaphoreGive(p->first);
      vTaskDelay(pdMS_TO_TICKS(100));
    } else {
      Serial.printf("[%s] TIMEOUT waiting for %s -> release %s, back off\n",
                    p->taskName, p->secondName, p->firstName);
      xSemaphoreGive(p->first);
      vTaskDelay(pdMS_TO_TICKS(random(100, 400)));   // random backoff
    }
  }
}

// Single-lock worker (Task_3): uses only B, short critical section.
// Holds at most one lock, so it can never be part of a circular wait -
// but it CAN be blocked by a deadlock that involves B.
void singleLockTask(void* pv) {
  LockPlan* p = (LockPlan*)pv;
  for (;;) {
    TickType_t t0 = xTaskGetTickCount();
    xSemaphoreTake(p->first, portMAX_DELAY);
    uint32_t waitedMs = (xTaskGetTickCount() - t0) * portTICK_PERIOD_MS;

    p->rounds++;
    if (waitedMs > 0) {
      Serial.printf("[%s] got %s after waiting %lu ms (round %lu)\n",
                    p->taskName, p->firstName,
                    (unsigned long)waitedMs, (unsigned long)p->rounds);
    }
    vTaskDelay(pdMS_TO_TICKS(20));    // short use of B
    xSemaphoreGive(p->first);
    vTaskDelay(pdMS_TO_TICKS(80));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.printf("\n=== Demo 1: Deadlock | DEMO_MODE %d ===\n", DEMO_MODE);

  mutexA = xSemaphoreCreateMutex();
  mutexB = xSemaphoreCreateMutex();
  configASSERT(mutexA && mutexB);

  initPlan(plan1, "Task_1", mutexA, "A", mutexB, "B");
#if DEMO_MODE == 2
  initPlan(plan2, "Task_2", mutexA, "A", mutexB, "B");   // same order as Task_1
#else
  initPlan(plan2, "Task_2", mutexA, "A", mutexB, "B");   // opposite order
#endif
  initPlan(plan3, "Task_3", mutexB, "B", NULL, NULL);    // B only, all modes

  xTaskCreatePinnedToCore(workerTask,     "Task_1", 4096, &plan1, 2, NULL, 1);
  xTaskCreatePinnedToCore(workerTask,     "Task_2", 4096, &plan2, 2, NULL, 1);
  xTaskCreatePinnedToCore(singleLockTask, "Task_3", 4096, &plan3, 2, NULL, 1);
}

// loop() is the heartbeat: if the rounds counters stop increasing, nothing progresses.
void loop() {
  delay(1000);
  Serial.printf("[HEARTBEAT] t=%lu s | rounds: Task_1=%lu Task_2=%lu Task_3=%lu\n",
                (unsigned long)(millis() / 1000),
                (unsigned long)plan1.rounds, (unsigned long)plan2.rounds,
                (unsigned long)plan3.rounds);
}