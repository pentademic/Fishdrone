/**
 * @file telemetry.ino
 * @brief Telemetry module — alert and position transmission — FishDrone
 *
 * Serialises alert messages and GPS coordinates into a compact ASCII frame
 * and transmits them over a UART-connected radio module (e.g. RFM95 LoRa,
 * HC-12, or any UART-bridged radio).
 *
 * Frame format (ASCII, '\n' terminated):
 *   ALERT,<lat>,<lon>,<bearing>,<distance_km>,<vessel_type>,<signal>\n
 *
 * Example:
 *   ALERT,43.296500,5.381300,127.0,3.45,fishing_trawler,72\n
 *
 * A heartbeat frame is sent every 10 s when no alert is active:
 *   HBEAT,<lat>,<lon>,<mode>\n
 *
 * Config: firmware/include/fishdrone_config.h
 */

#include <Arduino.h>
#include "../include/fishdrone_config.h"
#include "../include/fishdrone_types.h"

#ifndef TELEM_USE_SERIAL3
#include <SoftwareSerial.h>
static SoftwareSerial telemSerial(TELEM_SW_RX, TELEM_SW_TX);
#define TELEM_SERIAL telemSerial
#else
#define TELEM_SERIAL Serial3
#endif

// ─── Internal state ───────────────────────────────────────────────────────────
static uint32_t _last_heartbeat_ms = 0;
#define HEARTBEAT_INTERVAL_MS  10000UL

// ─── Public API ───────────────────────────────────────────────────────────────

/** Initialise the telemetry UART. Call once in setup(). */
void telemetry_init() {
    TELEM_SERIAL.begin(TELEMETRY_BAUD);
    Serial.println("[TELEM] Telemetry ready.");
}

/**
 * @brief Transmit an ALERT frame for a suspected illegal vessel.
 *
 * @param fix     Current GPS fix (position of the drone).
 * @param echo    Radar echo describing the target.
 * @param vessel  AI classification result.
 */
void telemetry_send_alert(const GpsFix*    fix,
                          const RadarEcho* echo,
                          const VesselInfo* vessel) {
    char frame[128];
    snprintf(frame, sizeof(frame),
             "ALERT,%.6f,%.6f,%.1f,%.2f,%s,%u",
             fix->valid    ? fix->latitude  : 0.0f,
             fix->valid    ? fix->longitude : 0.0f,
             echo->bearing_deg,
             echo->distance_km,
             vessel->vessel_type,
             echo->signal_strength);

    TELEM_SERIAL.println(frame);
    Serial.print("[TELEM] Sent alert: ");
    Serial.println(frame);
}

/**
 * @brief Non-blocking heartbeat — sends a position frame every 10 s.
 * Call every loop iteration.
 *
 * @param fix   Current GPS fix.
 * @param mode  Current drone operating mode.
 */
void telemetry_heartbeat(const GpsFix* fix, DroneMode mode) {
    if ((millis() - _last_heartbeat_ms) < HEARTBEAT_INTERVAL_MS) return;
    _last_heartbeat_ms = millis();

    const char* mode_str[] = {"STANDBY", "ACTIVE", "ALERT", "AVOID"};
    char frame[96];
    snprintf(frame, sizeof(frame),
             "HBEAT,%.6f,%.6f,%s",
             fix->valid ? fix->latitude  : 0.0f,
             fix->valid ? fix->longitude : 0.0f,
             mode_str[(int)mode]);

    TELEM_SERIAL.println(frame);
    Serial.print("[TELEM] Heartbeat: ");
    Serial.println(frame);
}

/**
 * @brief Non-blocking receive — reads any incoming command frames.
 *
 * Supported incoming commands (from base station):
 *   CMD,GOTO,<lat>,<lon>\n   — add an immediate waypoint
 *   CMD,STOP\n               — halt motors
 *   CMD,ACTIVE\n             — force wake from standby
 *
 * Dispatches to the relevant modules (declare externs below).
 */
void telemetry_receive() {
    static char   buf[64];
    static uint8_t idx = 0;

    while (TELEM_SERIAL.available()) {
        char c = (char)TELEM_SERIAL.read();
        if (c == '\n' || c == '\r') {
            if (idx > 0) {
                buf[idx] = '\0';
                Serial.print("[TELEM] Received: ");
                Serial.println(buf);

                if (strncmp(buf, "CMD,STOP", 8) == 0) {
                    extern void nav_stop();
                    nav_stop();
                } else if (strncmp(buf, "CMD,ACTIVE", 10) == 0) {
                    extern void power_force_active();
                    power_force_active();
                } else if (strncmp(buf, "CMD,GOTO,", 9) == 0) {
                    float lat, lon;
                    if (sscanf(buf + 9, "%f,%f", &lat, &lon) == 2) {
                        Waypoint wp = {lat, lon};
                        extern void nav_load_route(const Waypoint*, uint8_t);
                        nav_load_route(&wp, 1);
                        Serial.print("[TELEM] New waypoint: ");
                        Serial.print(lat, 6); Serial.print(", ");
                        Serial.println(lon, 6);
                    }
                }
                idx = 0;
            }
        } else if (idx < sizeof(buf) - 1) {
            buf[idx++] = c;
        } else {
            idx = 0;
        }
    }
}

// ─── Standalone test sketch ───────────────────────────────────────────────────

void setup() {
    Serial.begin(DEBUG_BAUD);
    while (!Serial) {}
    telemetry_init();
    Serial.println("[TELEM] Telemetry test — sending heartbeats every 10 s.");
}

void loop() {
    GpsFix fake_fix;
    fake_fix.valid     = true;
    fake_fix.latitude  = 43.2965f;
    fake_fix.longitude = 5.3813f;
    fake_fix.speed_knots = 1.2f;
    fake_fix.course_deg  = 90.0f;
    fake_fix.satellites  = 8;

    telemetry_heartbeat(&fake_fix, MODE_ACTIVE);
    telemetry_receive();
    delay(100);
}
