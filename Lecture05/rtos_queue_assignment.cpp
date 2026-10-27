#include <Arduino.h>

QueueHandle_t temperatureQueue;


// --------------------------------------------------
// Sensor Task
// --------------------------------------------------

void sensorTask(void *parameter)
{
  int temperature = 20;

  while (1)
  {
    // Send the temperature to the queue
    // Wait up to 100 ms if the queue is full
    if (xQueueSend(temperatureQueue, &temperature, pdMS_TO_TICKS(100)) != pdPASS)
    {
      Serial.println("Queue full, value dropped");
    }

    // Increase the temperature by 1
    temperature++;

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}


// --------------------------------------------------
// Display Task
// --------------------------------------------------

void displayTask(void *parameter)
{
  int receivedTemperature;

  while (1)
  {

    // Receive a temperature value from the queue
    if (xQueueReceive(temperatureQueue, &receivedTemperature, portMAX_DELAY) == pdPASS)
    {
      // Print the received value
      Serial.printf("Temperature: %d C\n", receivedTemperature);
    }

    // vTaskDelay(pdMS_TO_TICKS(100));
  }
}


// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup()
{
  Serial.begin(115200);

  // Create a queue that can store 5 integers
  temperatureQueue = xQueueCreate(5, sizeof(int));
  if (temperatureQueue == NULL)
  {
    Serial.println("Failed to create queue");
    while (1) { delay(1000); }
  }

  // Create the Sensor Task
  xTaskCreate(
    sensorTask,      // Task function
    "Sensor Task",   // Name
    2048,            // Stack size (bytes on ESP32)
    NULL,            // Parameter
    1,               // Priority
    NULL             // Task handle
  );

  // Create the Display Task
  xTaskCreate(
    displayTask,
    "Display Task",
    4096,            // More stack because of Serial.printf
    NULL,
    1,
    NULL
  );
}


void loop()
{
}
