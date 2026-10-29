/**
 * FreeRTOS Mutex Challenge - solution
 *
 * setup() passes the address of a local variable to a new task and uses a
 * mutex to block until the task has copied the value. Only then may setup()
 * return and its stack frame be reused.
 *
 * Based on starter by Shawn Hymel (0BSD)
 */
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const int led_pin = LED_BUILTIN;

// Mutex handle is global so both tasks can reach it (the parameter itself stays local)
static SemaphoreHandle_t mutex;

//*****************************************************************************
// Tasks

void blinkLED(void *parameters) {

  // Runs immediately on creation cause higher priority than setup,
  // so it gets the mutex before setup tries to take it
  xSemaphoreTake(mutex, portMAX_DELAY);
  int num = *(int *)parameters; // setup is blocked, stack still valid
  xSemaphoreGive(mutex);        // same task gives -> ownership respected

  Serial.print("Received: ");
  Serial.println(num);

  pinMode(led_pin, OUTPUT);

  while (1) {
    digitalWrite(led_pin, HIGH);
    vTaskDelay(pdMS_TO_TICKS(num));
    digitalWrite(led_pin, LOW);
    vTaskDelay(pdMS_TO_TICKS(num));
  }
}

//*****************************************************************************
// Main (runs as loopTask, priority 1)

void setup() {

  int delay_arg = 0;

  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));
  Serial.println();
  Serial.println("---FreeRTOS Mutex Challenge---");

  // Drop any stray bytes from boot/USB CDC
  while (Serial.available() > 0) Serial.read();

  // Re-prompt until positive number is given
  while (delay_arg <= 0) {
    Serial.println("Enter a number for delay (milliseconds)");
    while (Serial.available() <= 0) vTaskDelay(pdMS_TO_TICKS(10));
    delay_arg = Serial.parseInt();
    while (Serial.available() > 0) Serial.read();  // eat line ending
  }

  Serial.print("Sending: ");
  Serial.println(delay_arg);

  mutex = xSemaphoreCreateMutex();
  if (mutex == NULL) {
    Serial.println("Mutex creation failed");
    while (1) vTaskDelay(portMAX_DELAY);
  }

  // Same core as setup(), higher priority -> preempts us here
  xTaskCreatePinnedToCore(blinkLED,
                          "Blink LED",
                          2048,               // 1024 B is tight with Serial.print
                          (void *)&delay_arg,
                          uxTaskPriorityGet(NULL) + 1,
                          NULL,
                          xPortGetCoreID());

  // Block until the task gives the mutex back, i.e. has copied delay_arg
  xSemaphoreTake(mutex, portMAX_DELAY);
  xSemaphoreGive(mutex);

  Serial.println("Done!");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
