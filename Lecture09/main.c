#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

constexpr bool SLOW_TASK = true;
constexpr int LED_PIN = 4;
constexpr UBaseType_t QUEUE_LENGTH = 4;
constexpr uint32_t TASK_DELAY_MS = SLOW_TASK ? 500 : 20;

QueueHandle_t packageQueue = nullptr;
TaskHandle_t packageTaskHandle = nullptr;
hw_timer_t *packageTimer = nullptr;

void ARDUINO_ISR_ATTR onPackage() {
    // Owned by the ISR. Numbered from 1 and advanced on every interrupt,
    // even when the queue is full and the item is dropped.
    static uint32_t sequence = 0;
    ++sequence;

    // Each interrupt represents one package passing the sensor.
    // The queue stores a copy of the number. If the queue is full,
    // this call returns errQUEUE_FULL at once and the new item is dropped.
    BaseType_t taskWoken = pdFALSE;
    xQueueSendFromISR(packageQueue, &sequence, &taskWoken);

    if (taskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

void packageTask(void *) {
    uint32_t sequence = 0;

    while (true) {
        if (xQueueReceive(packageQueue, &sequence, portMAX_DELAY) == pdTRUE) {
            Serial.println(sequence);

            // the LED is on while the task is busy.
            digitalWrite(LED_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
            digitalWrite(LED_PIN, LOW);
        }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    packageQueue = xQueueCreate(QUEUE_LENGTH, sizeof(uint32_t));
    if (packageQueue == nullptr) {
        Serial.println("Queue creation failed.");
        return;
    }

#if ESP_ARDUINO_VERSION_MAJOR >= 3
    packageTimer = timerBegin(1000000);       // 1 MHz counter.
#else
    packageTimer = timerBegin(0, 80, true);   // 80 MHz APB / 80 = 1 MHz.
#endif
    if (packageTimer == nullptr) {
        Serial.println("Timer creation failed.");
        return;
    }

    if (xTaskCreate(packageTask, "Packages", 4096, nullptr, 2,
                    &packageTaskHandle) != pdPASS) {
        Serial.println("Task creation failed.");
        timerEnd(packageTimer);
        return;
    }

    Serial.println(SLOW_TASK ? "Run B: 500 ms task delay"
                              : "Run A: 20 ms task delay");
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    timerAttachInterrupt(packageTimer, onPackage);
    timerAlarm(packageTimer, 100000, true, 0);  // One interrupt every 100 ms.
#else
    timerAttachInterrupt(packageTimer, onPackage, true);
    timerAlarmWrite(packageTimer, 100000, true);  // 100 ms, auto-reload.
    timerAlarmEnable(packageTimer);
#endif
}

void loop() {
    delay(1000);
}