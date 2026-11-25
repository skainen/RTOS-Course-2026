/*
  Real Time Systems (4_EFS8050) - Lecture 10
  Demo 2 - Starvation

  OBJECTIVE
    Show that a ready task can get (almost) no CPU time when a higher-priority
    task never gives the CPU back, and how a short delay/yield fixes it.

  HARDWARE
    ESP32-S3 DevKitC-1 + USB cable. No other parts.

  HOW TO RUN
    Board: "ESP32S3 Dev Module". Serial Monitor: 115200 baud.
    If you use the board's native USB port, set Tools > USB CDC On Boot > Enabled.
    Run once with STARVATION_FIX 0, then set it to 1, upload, and compare.

  TASKS (all pinned to core 1, so priorities decide who runs)
    Task_Monitor  priority 3  prints a report every 500 ms
    Task_Hog      priority 2  busy for 3 s, then sleeps for 3 s (repeat)
    Task_Low      priority 1  counts (a small "work" step every tick)

  NOTE: STARVATION_FIX 0 is a teaching example, not production code.
*/
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define STARVATION_FIX 1     // 0 = Task_Hog never yields, 1 = Task_Hog yields

volatile uint32_t lowCount = 0;
volatile bool     hogBusy  = false;

void hogTask(void* pv) {
  for (;;) {
    hogBusy = true;
    uint32_t start = millis();
    uint32_t lastYield = start;
    while (millis() - start < 3000) {           // 3 s of "heavy work"
#if STARVATION_FIX
      if (millis() - lastYield >= 10) {         // yield every 10 ms
        vTaskDelay(10);                         // give lower priorities a chance
        lastYield = millis();
      }
#endif
    }
    hogBusy = false;
    vTaskDelay(pdMS_TO_TICKS(3000));            // sleep: CPU is free again
  }
}

void lowTask(void* pv) {
  for (;;) {
    lowCount++;                                  // one small step of work
    vTaskDelay(1);                               // wait one tick
  }
}

void monitorTask(void* pv) {
  uint32_t lastLow = 0;
  uint32_t t0 = millis();
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(500));
    uint32_t now = lowCount;
    Serial.printf("t=%4lu ms | Task_Hog: %-5s | Task_Low progress: +%lu\n",
                  (unsigned long)(millis() - t0),
                  hogBusy ? "BUSY" : "SLEEP",
                  (unsigned long)(now - lastLow));
    lastLow = now;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.printf("\n=== Demo 2: Starvation | STARVATION_FIX %d ===\n", STARVATION_FIX);

  xTaskCreatePinnedToCore(lowTask,     "Task_Low",     4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(hogTask,     "Task_Hog",     4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(monitorTask, "Task_Monitor", 4096, NULL, 3, NULL, 1);
}

void loop() {
  vTaskDelete(NULL);    // loop() is not needed; remove its task
}
