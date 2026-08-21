/**
 * @file tof_multi.ino
 * @brief Multi-sensor VL53L1X manager — FishDrone obstacle avoidance (5 sensors)
 *
 * Manages five VL53L1X TOF sensors on the same I²C bus by temporarily shutting
 * down each sensor at boot (via its XSDN pin) and reassigning a unique I²C
 * address before bringing the next one online.
 *
 * Sensor directions:
 *   FRONT  — forward-facing (collision avoidance)
 *   RIGHT  — starboard
 *   BACK   — stern
 *   LEFT   — port
 *   DOWN   — depth / water-surface clearance
 *
 * Wiring: see docs/wiring.md
 * Pin assignments: see firmware/include/fishdrone_config.h
 *
 * Library: VL53L1X by Pololu — install via Arduino Library Manager.
 */

#include <Wire.h>
#include <VL53L1X.h>
#include "../include/fishdrone_config.h"
#include "../include/fishdrone_types.h"

// ─── Sensor objects ───────────────────────────────────────────────────────────
static VL53L1X sensors[TOF_COUNT];

static const uint8_t XSDN_PINS[TOF_COUNT] = {
    TOF_XSDN_FRONT,
    TOF_XSDN_RIGHT,
    TOF_XSDN_BACK,
    TOF_XSDN_LEFT,
    TOF_XSDN_DOWN,
};

static const uint8_t ADDRESSES[TOF_COUNT] = {
    TOF_ADDR_FRONT,
    TOF_ADDR_RIGHT,
    TOF_ADDR_BACK,
    TOF_ADDR_LEFT,
    TOF_ADDR_DOWN,
};

static const char* NAMES[TOF_COUNT] = {
    "FRONT", "RIGHT", "BACK", "LEFT", "DOWN"
};

// ─── Public API ───────────────────────────────────────────────────────────────

/**
 * @brief Initialise all five TOF sensors with unique I²C addresses.
 * @return true if all sensors initialised successfully, false otherwise.
 */
bool tof_init() {
    Wire.begin();

    // Step 1 — pull all XSDN low to disable every sensor
    for (int i = 0; i < TOF_COUNT; i++) {
        pinMode(XSDN_PINS[i], OUTPUT);
        digitalWrite(XSDN_PINS[i], LOW);
    }
    delay(10);

    // Step 2 — bring each sensor up one at a time and assign its address
    for (int i = 0; i < TOF_COUNT; i++) {
        digitalWrite(XSDN_PINS[i], HIGH);
        delay(10);  // give sensor time to boot

        sensors[i].setTimeout(500);
        if (!sensors[i].init()) {
            Serial.print("[TOF] ERROR: Failed to init sensor ");
            Serial.println(NAMES[i]);
            return false;
        }

        // Reassign address (skip for the first sensor which keeps 0x29)
        if (ADDRESSES[i] != 0x29) {
            sensors[i].setAddress(ADDRESSES[i]);
        }

        sensors[i].setDistanceMode(VL53L1X::Long);
        sensors[i].setMeasurementTimingBudget(33000);  // 33 ms for faster polling
        sensors[i].startContinuous(33);

        Serial.print("[TOF] Sensor ");
        Serial.print(NAMES[i]);
        Serial.print(" initialised at address 0x");
        Serial.println(ADDRESSES[i], HEX);
    }

    return true;
}

/**
 * @brief Read all five TOF sensors and populate a TofReadings struct.
 * @param[out] readings  Pointer to TofReadings to fill.
 */
void tof_read_all(TofReadings* readings) {
    readings->front_mm = sensors[0].read(false);
    readings->right_mm = sensors[1].read(false);
    readings->back_mm  = sensors[2].read(false);
    readings->left_mm  = sensors[3].read(false);
    readings->down_mm  = sensors[4].read(false);

    for (int i = 0; i < TOF_COUNT; i++) {
        if (sensors[i].timeoutOccurred()) {
            Serial.print("[TOF] Timeout on sensor ");
            Serial.println(NAMES[i]);
        }
    }
}

/**
 * @brief Check whether any sensor reading is below the obstacle threshold.
 * @param readings  Populated TofReadings struct.
 * @return true if an obstacle is detected in any direction.
 */
bool tof_obstacle_detected(const TofReadings* readings) {
    return (readings->front_mm < TOF_OBSTACLE_THRESHOLD_MM ||
            readings->right_mm < TOF_OBSTACLE_THRESHOLD_MM ||
            readings->back_mm  < TOF_OBSTACLE_THRESHOLD_MM ||
            readings->left_mm  < TOF_OBSTACLE_THRESHOLD_MM);
    // Down sensor is excluded from collision detection — used for depth only.
}

/**
 * @brief Print all readings to Serial (debug helper).
 */
void tof_print(const TofReadings* readings) {
    Serial.print("[TOF] F:");   Serial.print(readings->front_mm);
    Serial.print(" R:");        Serial.print(readings->right_mm);
    Serial.print(" B:");        Serial.print(readings->back_mm);
    Serial.print(" L:");        Serial.print(readings->left_mm);
    Serial.print(" D:");        Serial.print(readings->down_mm);
    Serial.println(" mm");
}

// ─── Standalone test sketch ───────────────────────────────────────────────────
// Remove or comment out setup()/loop() when integrating into fishdrone_main.ino

void setup() {
    Serial.begin(DEBUG_BAUD);
    while (!Serial) {}
    Serial.println("[TOF] Multi-sensor init...");
    if (!tof_init()) {
        Serial.println("[TOF] Init failed — halting.");
        while (1) {}
    }
    Serial.println("[TOF] All sensors ready.");
}

void loop() {
    TofReadings r;
    tof_read_all(&r);
    tof_print(&r);

    if (tof_obstacle_detected(&r)) {
        Serial.println("[TOF] *** OBSTACLE DETECTED ***");
    }

    delay(100);
}
