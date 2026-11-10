#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_arduino_version.h>

// ---------------- Hardware configuration ----------------
#define OLED_SDA_PIN   8       
#define OLED_SCL_PIN   9
#define OLED_ADDR      0x3C
#define OLED_W         128
#define OLED_H         64

#define RGB_LED_PIN    38      
#define LED_LEVEL      10      // per-channel brightness 0-255

// ---------------- Timing ----------------
static const uint32_t SESSION_MS = 10000;
static const uint32_t TICK_MS    = 1000;
static const uint32_t POLL_MS    = 20;     // max wait before re-checking Serial
static const TickType_t CMD_WAIT = pdMS_TO_TICKS(10);  // timer command queue wait

// ---------------- Notification bits ----------------
static const uint32_t EVT_UPDATE   = (1UL << 0);
static const uint32_t EVT_COMPLETE = (1UL << 1);

enum class State : uint8_t { Ready, Running, Complete };

static Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);

static TimerHandle_t tickTimer     = nullptr;   // auto-reload
static TimerHandle_t sessionTimer  = nullptr;   // one-shot
static TaskHandle_t  appTaskHandle = nullptr;

// ---------------- Timer callbacks ----------------
static void tickTimerCb(TimerHandle_t) {
  if (appTaskHandle) xTaskNotify(appTaskHandle, EVT_UPDATE, eSetBits);
}

static void sessionTimerCb(TimerHandle_t) {
  if (appTaskHandle) xTaskNotify(appTaskHandle, EVT_COMPLETE, eSetBits);
}


// ---------------- Output helpers ----------------
static void ledSet(bool on) {
  uint8_t v = on ? LED_LEVEL : 0;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  rgbLedWrite(RGB_LED_PIN, v, v, v);
#else
  neopixelWrite(RGB_LED_PIN, v, v, v);
#endif
}

static void drawScreen(const char *line1, const char *line2) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 16);
  display.print(line1);
  if (line2) {
    display.setCursor(0, 36);
    display.print(line2);
  }
  display.display();
}

static void showReady()    { drawScreen("Ready", "Send s to start"); }
static void showComplete() { drawScreen("Session complete", nullptr); }

static void showRunning(uint32_t seconds) {
  char buf[24];
  snprintf(buf, sizeof buf, "Remaining: %lu s", (unsigned long)seconds);
  drawScreen("Session running", buf);
}

static void fatal(const char *msg) {
  Serial.println(msg);
  for (;;) vTaskDelay(portMAX_DELAY);
}


// ---------------- Session logic ----------------
static uint32_t elapsedMs(TickType_t startTick) {
  return pdTICKS_TO_MS(xTaskGetTickCount() - startTick);
}

static uint32_t remainingSeconds(TickType_t startTick) {
  uint32_t e = elapsedMs(startTick);
  if (e >= SESSION_MS) return 0;
  return (SESSION_MS - e + 500) / 1000;
}

static bool startSession(TickType_t &startTick) {
  ulTaskNotifyValueClear(nullptr, UINT32_MAX);   // drop any stale events

  startTick = xTaskGetTickCount();

  if (xTimerStart(sessionTimer, CMD_WAIT) != pdPASS) {
    Serial.println("Error: session timer start failed");
    return false;
  }
  if (xTimerStart(tickTimer, CMD_WAIT) != pdPASS) {
    Serial.println("Error: tick timer start failed");
    if (xTimerStop(sessionTimer, CMD_WAIT) != pdPASS)
      Serial.println("Error: session timer stop failed");
    return false;
  }
  return true;
}

// ---------------- Application tasks ----------------
static void appTask(void *) {
  appTaskHandle = xTaskGetCurrentTaskHandle();

  State      state     = State::Ready;
  TickType_t startTick = 0;
  uint32_t   shownSec  = UINT32_MAX;

  for (;;) {
    uint32_t bits = 0;
    // Bounded wait: returns immediately on a timer event, otherwise after
    // POLL_MS so Serial input is still serviced.
    xTaskNotifyWait(0, UINT32_MAX, &bits, pdMS_TO_TICKS(POLL_MS));

    // 1) Completion has priority over update
    if ((bits & EVT_COMPLETE) && state == State::Running) {
      if (xTimerStop(tickTimer, CMD_WAIT) != pdPASS)
        Serial.println("Error: tick timer stop failed");
      ledSet(false);
      showComplete();
      state = State::Complete;
      bits &= ~EVT_UPDATE;   // a simultaneous update is discarded
      Serial.printf("[%lu ms] Session complete\n",
                    (unsigned long)elapsedMs(startTick));
    }

    // 2) Countdown update
    if ((bits & EVT_UPDATE) && state == State::Running) {
      uint32_t r = remainingSeconds(startTick);
      if (r != shownSec) {
        showRunning(r);
        shownSec = r;
      }
      Serial.printf("[%lu ms] Remaining: %lu s\n",
                    (unsigned long)elapsedMs(startTick), (unsigned long)r);
    }

    // 3) Serial command
    while (Serial.available() > 0) {
      int c = Serial.read();
      if (c != 's') continue;

      if (state == State::Running) {
        Serial.printf("[%lu ms] Session already running\n",
                      (unsigned long)elapsedMs(startTick));
        continue;
      }

      if (startSession(startTick)) {
        state    = State::Running;
        shownSec = SESSION_MS / 1000;
        ledSet(true);
        showRunning(shownSec);
        Serial.println("[0 ms] Session started");
      } else {
        Serial.println("Session not started");
      }
    }
  }
}


void setup() {
  Serial.begin(115200);

  ledSet(false);

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  Wire.setClock(400000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
    fatal("SSD1306 init failed");
  showReady();

  // Created dormant; started only when a session begins.
  tickTimer    = xTimerCreate("tick",    pdMS_TO_TICKS(TICK_MS),    pdTRUE,
                              nullptr, tickTimerCb);
  sessionTimer = xTimerCreate("session", pdMS_TO_TICKS(SESSION_MS), pdFALSE,
                              nullptr, sessionTimerCb);
  if (tickTimer == nullptr || sessionTimer == nullptr)
    fatal("Timer creation failed");

  Serial.println("Ready. Send 's' to start.");

  if (xTaskCreatePinnedToCore(appTask, "app", 4096, nullptr, 2, nullptr,
                              ARDUINO_RUNNING_CORE) != pdPASS)
    fatal("App task creation failed");
}

void loop() {
  vTaskDelete(nullptr);   // all work is in appTask
}