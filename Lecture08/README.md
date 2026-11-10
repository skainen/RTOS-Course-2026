# ESP32-S3 Study Session Timer

A 10-second study timer using two FreeRTOS software timers. Send `s` over Serial to start a session. The OLED shows the countdown and the onboard LED is on while the session runs.

## Wiring

- OLED VCC: 3V3
- OLED GND: GND
- OLED SDA: GPIO 8
- OLED SCL: GPIO 9
- Onboard RGB LED: GPIO 38

## Libraries

- Adafruit SSD1306
- Adafruit GFX

## Board settings

- Board: ESP32S3
- Serial Monitor: 115200 baud

## Test results

1. Restart the board and wait over 10 seconds
   - Expected: Ready screen remains visible, LED stays off
   - Observed: Working as expected

2. Send `s`
   - Expected: Session starts, countdown appears, LED turns on
   - Observed: Working as expected.

3. Send `s` repeatedly during the session
   - Expected: Original session continues without extending
   - Observed: 10 second counter keeps on decreasing as intended, with current time being printed in terminal

4. Allow the session to finish
   - Expected: Completion screen appears and LED turns off after 10 seconds
   - Observed: Working as expected

5. Wait after completion
   - Expected: Completion screen remains visible
   - Observed: Working as expected

6. Send `s` again
   - Expected: A new session starts with the full duration
   - Observed: Working as expected.

## Questions

**Why does this system need both a one-shot and an auto-reload timer?**
The auto-reload timer triggers a display update every second. The one-shot timer marks the end of the session once, after 10 seconds, so the session length doesn't depend on counting ticks.

**Which task executes the timer callbacks?**
The FreeRTOS timer service task (daemon task).

**Why do OLED updates belong in the application task?**
OLED updates are relatively slow I2C operations. Running them in a callback would block the timer service task and delay all other timers.

**When are the timers dormant?**
Before the first session, after the one-shot timer expires, and after the auto-reload timer is stopped when the session completes.

**How does your program prevent extra `s` commands from extending the session?**
The application task ignores `s` while the state is Running, so the timers are never restarted and the start time is never reset.