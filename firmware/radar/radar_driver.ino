/**
 * @file radar_driver.ino
 * @brief Radar interface for FishDrone — UART protocol with Nucleo simulator
 *
 * The physical radar is simulated by a second STM32 Nucleo board that sends
 * structured text frames over UART.
 *
 * Frame format (ASCII, terminated with '\n'):
 *   RADAR,<detected>,<distance_km>,<bearing_deg>,<signal_strength>\n
 *
 * Example frames:
 *   RADAR,1,3.45,127,72\n   → vessel at 3.45 km, bearing 127°, signal 72
 *   RADAR,0,0,0,0\n         → no echo
 *
 * The driver exposes:
 *   radar_init()         — begin UART
 *   radar_update()       — non-blocking read / parse
 *   radar_get_echo()     — retrieve latest RadarEcho
 *
 * Pin / baud: firmware/include/fishdrone_config.h
 */

#include "../include/fishdrone_config.h"
#include "../include/fishdrone_types.h"

#ifndef RADAR_USE_SERIAL2
#include <SoftwareSerial.h>
static SoftwareSerial radarSerial(RADAR_SW_RX, RADAR_SW_TX);
#define RADAR_SERIAL radarSerial
#else
#define RADAR_SERIAL Serial2
#endif

// ─── Internal state ───────────────────────────────────────────────────────────
static RadarEcho _echo;
static char      _buf[64];
static uint8_t   _buf_idx = 0;

// ─── Frame parser ─────────────────────────────────────────────────────────────

/**
 * Parse a complete "RADAR,..." frame into the _echo struct.
 * Returns true on success.
 */
static bool parse_frame(const char* frame) {
    if (strncmp(frame, "RADAR,", 6) != 0) return false;

    char tmp[64];
    strncpy(tmp, frame + 6, sizeof(tmp));
    tmp[sizeof(tmp) - 1] = '\0';

    char* token = strtok(tmp, ",");
    if (!token) return false;
    _echo.detected = (atoi(token) != 0);

    token = strtok(nullptr, ",");
    if (!token) return false;
    _echo.distance_km = atof(token);

    token = strtok(nullptr, ",");
    if (!token) return false;
    _echo.bearing_deg = atof(token);

    token = strtok(nullptr, ",");
    if (!token) return false;
    _echo.signal_strength = (uint8_t)atoi(token);

    return true;
}

// ─── Public API ───────────────────────────────────────────────────────────────

/** Initialise the radar UART. Call once in setup(). */
void radar_init() {
    RADAR_SERIAL.begin(RADAR_BAUD);
    memset(&_echo, 0, sizeof(_echo));
    Serial.println("[RADAR] Driver ready.");
}

/**
 * @brief Non-blocking update — call every loop iteration.
 * Reads bytes from the radar UART and parses complete frames.
 */
void radar_update() {
    while (RADAR_SERIAL.available()) {
        char c = (char)RADAR_SERIAL.read();
        if (c == '\n' || c == '\r') {
            if (_buf_idx > 0) {
                _buf[_buf_idx] = '\0';
                if (!parse_frame(_buf)) {
                    Serial.print("[RADAR] Bad frame: ");
                    Serial.println(_buf);
                }
                _buf_idx = 0;
            }
        } else if (_buf_idx < sizeof(_buf) - 1) {
            _buf[_buf_idx++] = c;
        } else {
            _buf_idx = 0;  // overflow — discard
        }
    }
}

/** Return the latest radar echo (read-only pointer). */
const RadarEcho* radar_get_echo() {
    return &_echo;
}

/** Return true if the last echo is above the detection threshold. */
bool radar_vessel_detected() {
    return _echo.detected &&
           (_echo.signal_strength >= RADAR_DETECT_THRESHOLD);
}

/** Print latest echo to Serial (debug). */
void radar_print() {
    if (_echo.detected) {
        Serial.print("[RADAR] Vessel @ ");
        Serial.print(_echo.distance_km);
        Serial.print(" km, bearing ");
        Serial.print(_echo.bearing_deg);
        Serial.print("°, signal ");
        Serial.println(_echo.signal_strength);
    } else {
        Serial.println("[RADAR] No echo.");
    }
}

// ─── Nucleo simulator sketch ──────────────────────────────────────────────────
// Flash this code to the second Nucleo board to simulate radar output.
// Uncomment RADAR_SIMULATOR before compiling for the Nucleo simulator.
//
// #define RADAR_SIMULATOR
#ifdef RADAR_SIMULATOR

#include <stdlib.h>

void setup() {
    Serial.begin(RADAR_BAUD);  // Connect Nucleo TX to FishDrone radar RX
}

void loop() {
    static uint32_t t = 0;
    if (millis() - t >= 500) {
        t = millis();
        // Simulate a vessel appearing every ~10 s
        bool det = ((millis() / 1000) % 10 < 3);
        float dist    = det ? (float)(rand() % 500 + 100) / 100.0f : 0.0f;
        float bearing = det ? (float)(rand() % 360) : 0.0f;
        int   signal  = det ? (rand() % 60 + 20) : 0;
        Serial.print("RADAR,");
        Serial.print(det ? 1 : 0); Serial.print(",");
        Serial.print(dist, 2);     Serial.print(",");
        Serial.print(bearing, 1);  Serial.print(",");
        Serial.println(signal);
    }
}

#endif  // RADAR_SIMULATOR

// ─── Standalone test sketch ───────────────────────────────────────────────────

#ifndef RADAR_SIMULATOR

void setup() {
    Serial.begin(DEBUG_BAUD);
    while (!Serial) {}
    radar_init();
}

void loop() {
    radar_update();
    radar_print();
    delay(500);
}

#endif
