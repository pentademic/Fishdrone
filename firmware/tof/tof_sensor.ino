/**
 * @file tof_sensor.ino
 * @brief VL53L1X Time-of-Flight sensor driver — FishDrone obstacle avoidance
 *
 * Reads distance from a single VL53L1X sensor over I²C and prints the result
 * to the Serial monitor at 115200 baud.
 *
 * Hardware:
 *   - STM32 Nucleo board (or compatible Arduino)
 *   - ST VL53L1 satellite board connected via I²C
 *
 * Wiring (see docs/wiring.md for full details):
 *   VL53L1 pin 1 (INT)    → A2
 *   VL53L1 pin 2 (SCL_I)  → D15 (SCL)  [4.7 kΩ pull-up to 3.3 V]
 *   VL53L1 pin 3 (XSDN_I) → A1
 *   VL53L1 pin 4 (SDA_I)  → D14 (SDA)  [4.7 kΩ pull-up to 3.3 V]
 *   VL53L1 pin 5 (VDD)    → 3V3
 *   VL53L1 pin 6 (GND)    → GND
 *
 * Library: VL53L1X by Pololu — install via Arduino Library Manager.
 *
 * For the multi-sensor setup (5× VL53L1X) see the VL53L1_Sat_HelloWorld
 * examples bundled with the X-NUCLEO-53L1A1 expansion board package.
 */

#include <Wire.h>
#include <VL53L1X.h>

VL53L1X sensor;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!sensor.init()) {
    Serial.println("Error: Could not initialise VL53L1X sensor!");
    while (1);
  }

  // Long mode supports distances up to ~4 m outdoors / 8 m in darkness.
  sensor.setDistanceMode(VL53L1X::Long);

  // Timing budget: longer = more accurate but slower (50 ms recommended).
  sensor.setMeasurementTimingBudget(50000);

  sensor.startContinuous();
}

void loop() {
  int distance = sensor.read();

  if (sensor.timeoutOccurred()) {
    Serial.println("Sensor timeout!");
  } else {
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" mm");
  }

  delay(100);
}
