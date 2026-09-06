Smart Silo & Tank Monitoring System - 2 Hour Deep Sleep

An IoT solution for automated level monitoring of feed silos, water tanks, and liquid fertilizer tanks using an ultrasonic sensor and ESP32. The device wakes up every 2 hours, sends real-time Telegram alerts, and goes back to deep sleep to save power and prevent unexpected shortages.

Problem

Silos and tanks are often checked manually and irregularly, which can lead to feed, water, or fertilizer running out unexpectedly. This project automates that monitoring so farmers get notified in time to reorder.

How It Works

- An ultrasonic sensor `HC-SR04 / JSN-SR04T` is mounted at the top of the silo/tank, facing down, and measures the distance to the material/liquid surface.
- The ESP32 converts this distance into a fill percentage based on the tank's known height.
- To save power, the device wakes up every 2 hours, takes 5 readings, sends a report, checks thresholds, and goes back to deep sleep.
- When the level drops to a configurable threshold `default 20%`, the system sends an automatic Telegram alert.
- A second, more urgent alert triggers at a critical threshold `default 10%`.
- The same system works for feed silos, water tanks, or liquid fertilizer tanks by only changing the tank height in code.

Hardware
Component	Role
ESP32	Microcontroller, WiFi connectivity, Deep Sleep
HC-SR04 / JSN-SR04T Ultrasonic Sensor	Distance measurement
Jumper Wires	Direct connection
Wiring - Direct 3.3V Connection

We are powering the sensor directly from 3.3V so no voltage divider or resistors are needed.
Sensor Pin	ESP32 Pin
VCC	3.3V
GND	GND
TRIG	GPIO 5
ECHO	GPIO 18
`Note`: Most HC-SR04 and JSN-SR04T modules work fine on 3.3V. Direct connection to ECHO is safe because both are 3.3V logic.

Software

- Arduino IDE / ESP32 core
- Libraries: `UniversalTelegramBot`, `ArduinoJson` `v6`
- Deep Sleep mode: Wakes up every 2 hours to measure
- Averages 5 ultrasonic readings to filter noise and rejects out-of-range values
- Threshold alerts prevent repeated spam

Setup

1. Install the required libraries via the Arduino Library Manager.
2. Create a Telegram bot via @BotFather and get your bot token and chat ID.
3. Measure the empty distance from the sensor to the bottom of the tank and update `SILO_HEIGHT_CM`.
4. Enter your WiFi credentials and Telegram bot token in the config section.
5. Flash to the ESP32. The device will wake up every 2 hours automatically.

Future Improvements

- Multi-sensor support for tracking multiple silos/tanks from one board
- Data logging to a dashboard `Node-RED / InfluxDB / Grafana`
- Solar power option for off-grid installations
