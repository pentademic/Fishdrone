/**
 * @file power_mgmt.ino
 * @brief Power management — sleep/wake state machine — FishDrone
 *
 * Implements the two-mode power strategy:
 *
 *   ACTIVE mode   — all subsystems running (radar, TOF, GPS, camera, motors)
 *   STANDBY mode  — only radar + TOF polling; camera/GPS/motors suspended
 *
 * Transition rules:
 *   ACTIVE  → STANDBY  : no radar echo AND no TOF obstacle for POWER_STANDBY_TIMEOUT_MS
 *   STANDBY → ACTIVE   : radar detects vessel OR any TOF reading < threshold
 *
 * On Arduino/STM32duino this is implemented by enabling/disabling peripheral
 * power and reducing polling rates rather than deep CPU sleep, to keep the
 * main loop running and allow instant wake-up.
 *
 * For true MCU sleep (STM32 STOP mode), use STM32LowPower library and enable
 * the WAKEUP_FROM_SLEEP define below.
 *
 * Config: firmware/include/fishdrone_config.h
 */

#include <Arduino.h>
#include "../include/fishdrone_config.h"
#include "../include/fishdrone_types.h"

// Uncomment to enable STM32 low-power STOP mode (requires STM32LowPower lib)
// #define WAKEUP_FROM_SLEEP
#ifdef WAKEUP_FROM_SLEEP
#include <STM32LowPower.h>
#endif

// ─── Internal state ───────────────────────────────────────────────────────────
static DroneMode _current_mode    = MODE_ACTIVE;
static uint32_t  _last_active_ms  = 0;  // millis() when a trigger was last seen

// ─── Forward declarations for subsystem enable/disable ───────────────────────
// These are no-ops in this stub; replace with real peripheral power gating
// (e.g. switching a load switch GPIO, pausing a timer, stopping Serial).
static void subsystem_camera_enable(bool en) {
    // TODO: toggle camera power via a GPIO load switch if fitted
    Serial.print("[PWR] Camera ");
    Serial.println(en ? "ON" : "OFF");
}

static void subsystem_gps_enable(bool en) {
    Serial.print("[PWR] GPS ");
    Serial.println(en ? "ON" : "OFF");
}

static void subsystem_motors_enable(bool en) {
    // Stopping motors is done via nav_stop(); here we just log the intent
    Serial.print("[PWR] Motors ");
    Serial.println(en ? "ENABLED" : "DISABLED");
}

// ─── Public API ───────────────────────────────────────────────────────────────

/** Initialise power management. Call once in setup(). */
void power_init() {
    _last_active_ms = millis();
    _current_mode   = MODE_ACTIVE;

#ifdef WAKEUP_FROM_SLEEP
    LowPower.begin();
#endif

    Serial.println("[PWR] Power management ready. Mode: ACTIVE");
}

/** Return the current operating mode. */
DroneMode power_get_mode() {
    return _current_mode;
}

/**
 * @brief Non-blocking power management update — call every loop iteration.
 *
 * @param radar_triggered  true if radar detected a vessel this cycle.
 * @param tof_triggered    true if any TOF sensor is below threshold this cycle.
 */
void power_update(bool radar_triggered, bool tof_triggered) {
    bool triggered = radar_triggered || tof_triggered;

    if (triggered) {
        _last_active_ms = millis();
    }

    switch (_current_mode) {

    case MODE_ACTIVE:
    case MODE_ALERT:
    case MODE_AVOID:
        // Check for transition to standby
        if ((millis() - _last_active_ms) >= POWER_STANDBY_TIMEOUT_MS) {
            Serial.println("[PWR] Entering STANDBY mode.");
            _current_mode = MODE_STANDBY;
            subsystem_camera_enable(false);
            subsystem_gps_enable(false);
            subsystem_motors_enable(false);

#ifdef WAKEUP_FROM_SLEEP
            // Configure TOF interrupt to wake the MCU
            // (requires an INT pin wired from one VL53L1X)
            LowPower.attachInterruptWakeup(TOF_XSDN_FRONT, NULL, CHANGE);
            LowPower.deepSleep();
#endif
        }
        break;

    case MODE_STANDBY:
        // Check for wake trigger
        if (triggered) {
            Serial.println("[PWR] Wake trigger — entering ACTIVE mode.");
            _current_mode = MODE_ACTIVE;
            subsystem_camera_enable(true);
            subsystem_gps_enable(true);
            subsystem_motors_enable(true);
            _last_active_ms = millis();
        }
        break;
    }
}

/** Force immediate transition to ACTIVE mode (e.g. after user command). */
void power_force_active() {
    _last_active_ms = millis();
    if (_current_mode == MODE_STANDBY) {
        _current_mode = MODE_ACTIVE;
        subsystem_camera_enable(true);
        subsystem_gps_enable(true);
        subsystem_motors_enable(true);
        Serial.println("[PWR] Forced ACTIVE mode.");
    }
}

/** Print current mode to Serial (debug). */
void power_print() {
    const char* names[] = {"STANDBY", "ACTIVE", "ALERT", "AVOID"};
    Serial.print("[PWR] Mode: ");
    Serial.println(names[(int)_current_mode]);
}

// ─── Standalone test sketch ───────────────────────────────────────────────────

void setup() {
    Serial.begin(DEBUG_BAUD);
    while (!Serial) {}
    power_init();
}

void loop() {
    // Simulate triggers fading after 35 s to force standby
    bool fake_radar = (millis() < 5000);
    bool fake_tof   = false;

    power_update(fake_radar, fake_tof);
    power_print();
    delay(1000);
}
