/**
 * @file fishdrone_main.ino
 * @brief Top-level integration sketch — FishDrone
 *
 * This is the entry point for the complete FishDrone firmware.
 * It ties together all subsystem modules:
 *
 *   ┌──────────────────────────────────────────────────┐
 *   │  Subsystem modules (each in their own folder)    │
 *   │                                                  │
 *   │  tof/tof_multi.ino       — 5× VL53L1X sensors   │
 *   │  gps/gps_driver.ino      — TESEO-LIV3F NMEA     │
 *   │  radar/radar_driver.ino  — Nucleo radar sim      │
 *   │  navigation/navigation.ino — PID waypoint ctrl   │
 *   │  avoidance/avoidance.ino — TOF obstacle avoid    │
 *   │  power/power_mgmt.ino    — sleep/wake FSM        │
 *   │  telemetry/telemetry.ino — alert TX + CMD RX     │
 *   │  camera/camera_driver.ino — turret + AI          │
 *   └──────────────────────────────────────────────────┘
 *
 * ── How to build ──────────────────────────────────────────────────────────────
 * Open THIS file (fishdrone_main.ino) in Arduino IDE.
 * The IDE automatically includes all other .ino files in the same sketch
 * folder. However, since modules live in sub-folders, you have two options:
 *
 * Option A (recommended for Arduino IDE):
 *   Copy all .ino files into a single folder named "fishdrone_main" and open
 *   fishdrone_main.ino. The IDE will compile them together.
 *   See firmware/README.md for the copy command.
 *
 * Option B (STM32CubeIDE):
 *   Add each .ino / .c file to the STM32CubeIDE project and compile as C++.
 *   See firmware/README.md for detailed instructions.
 *
 * ── Main loop overview ────────────────────────────────────────────────────────
 *
 *  setup()
 *    └─ Initialise all subsystems
 *
 *  loop()
 *    ├─ gps_update()              — parse incoming GPS bytes
 *    ├─ radar_update()            — parse incoming radar frames
 *    ├─ tof_read_all()            — read 5 TOF sensors
 *    ├─ power_update()            — manage sleep/wake
 *    │
 *    ├─ [STANDBY mode]
 *    │   └─ minimal polling only
 *    │
 *    └─ [ACTIVE / ALERT / AVOID mode]
 *        ├─ avoidance_update()    — check TOF, override nav if needed
 *        ├─ camera_update()       — capture + AI classify
 *        ├─ handle_alert()        — send telemetry if illegal vessel
 *        ├─ nav_update()          — PID to next waypoint (if path clear)
 *        └─ telemetry_heartbeat() — periodic position report
 *
 * Config: firmware/include/fishdrone_config.h
 */

#include "../include/fishdrone_config.h"
#include "../include/fishdrone_types.h"

// ─── Forward declarations (defined in sub-module .ino files) ─────────────────

// TOF
bool             tof_init();
void             tof_read_all(TofReadings*);
bool             tof_obstacle_detected(const TofReadings*);
void             tof_print(const TofReadings*);

// GPS
void             gps_init();
void             gps_update();
const GpsFix*    gps_get_fix();
void             gps_print();

// Radar
void             radar_init();
void             radar_update();
const RadarEcho* radar_get_echo();
bool             radar_vessel_detected();
void             radar_print();

// Navigation
void             nav_init();
void             nav_load_route(const Waypoint*, uint8_t);
void             nav_update(const GpsFix*);
void             nav_stop();
void             nav_apply_command(const MotorCommand*);

// Avoidance
void             avoidance_update(const TofReadings*, DroneMode*);
bool             avoidance_is_active();

// Power
void             power_init();
DroneMode        power_get_mode();
void             power_update(bool radar_triggered, bool tof_triggered);
void             power_force_active();

// Telemetry
void             telemetry_init();
void             telemetry_send_alert(const GpsFix*, const RadarEcho*, const VesselInfo*);
void             telemetry_heartbeat(const GpsFix*, DroneMode);
void             telemetry_receive();

// Camera
void             camera_init();
void             camera_update();
void             camera_aim(float bearing_deg, float own_course);
void             camera_home();
const VesselInfo* camera_get_vessel();
void             camera_clear_vessel();

// ─── Global state ─────────────────────────────────────────────────────────────
static DroneMode   g_mode = MODE_ACTIVE;
static TofReadings g_tof;
static bool        g_alert_sent = false;

// ─── Default patrol route ─────────────────────────────────────────────────────
// Replace these coordinates with your actual patrol waypoints.
static const Waypoint g_patrol_route[] = {
    {43.296500f,  5.381300f},   // WP0 — patrol centre
    {43.290000f,  5.370000f},   // WP1 — SW corner
    {43.285000f,  5.390000f},   // WP2 — SE corner
    {43.296500f,  5.400000f},   // WP3 — NE corner
};
static const uint8_t g_route_len = sizeof(g_patrol_route) / sizeof(g_patrol_route[0]);

// ─── Alert handler ────────────────────────────────────────────────────────────
static void handle_alert() {
    const RadarEcho*  echo   = radar_get_echo();
    const VesselInfo* vessel = camera_get_vessel();
    const GpsFix*     fix    = gps_get_fix();

    if (!vessel->classified || !vessel->suspected_illegal) {
        g_alert_sent = false;
        return;
    }
    if (g_alert_sent) return;  // avoid flooding

    Serial.println("[MAIN] *** ILLEGAL VESSEL DETECTED — sending alert ***");
    g_mode = MODE_ALERT;

    // Aim camera at the target
    if (fix->valid) {
        camera_aim(echo->bearing_deg, fix->course_deg);
    }

    telemetry_send_alert(fix, echo, vessel);
    g_alert_sent = true;

    // Clear after transmission so we can detect the next vessel
    // (re-arm after 30 s to avoid duplicate alerts for the same target)
    static uint32_t alert_time = 0;
    if (millis() - alert_time > 30000UL) {
        camera_clear_vessel();
        g_alert_sent = false;
        alert_time   = millis();
    }
}

// ─── setup() ──────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(DEBUG_BAUD);
    while (!Serial && millis() < 3000) {}  // wait up to 3 s for Serial Monitor

    Serial.println("==========================================");
    Serial.println("  F.I.S.H D.R.O.N.E — Firmware v1.0     ");
    Serial.println("==========================================");

    // Initialise subsystems
    if (!tof_init()) {
        Serial.println("[MAIN] FATAL: TOF init failed.");
        while (1) {}
    }

    gps_init();
    radar_init();
    nav_init();
    power_init();
    telemetry_init();
    camera_init();

    // Load patrol route
    nav_load_route(g_patrol_route, g_route_len);

    Serial.println("[MAIN] All subsystems ready. Entering patrol loop.");
    g_mode = MODE_ACTIVE;
}

// ─── loop() ───────────────────────────────────────────────────────────────────
void loop() {
    // ── 1. Update all sensor drivers (non-blocking) ───────────────────────────
    gps_update();
    radar_update();
    tof_read_all(&g_tof);
    telemetry_receive();

    const GpsFix*    fix  = gps_get_fix();
    const RadarEcho* echo = radar_get_echo();

    bool radar_trigger = radar_vessel_detected();
    bool tof_trigger   = tof_obstacle_detected(&g_tof);

    // ── 2. Power management ───────────────────────────────────────────────────
    power_update(radar_trigger, tof_trigger);
    g_mode = power_get_mode();

    // ── 3. Mode-specific logic ────────────────────────────────────────────────
    if (g_mode == MODE_STANDBY) {
        // Minimal polling in standby — no camera, no nav, no motor activity
        delay(500);
        return;
    }

    // ── 4. Obstacle avoidance (highest priority, overrides navigation) ─────────
    avoidance_update(&g_tof, &g_mode);

    // ── 5. Camera + AI (only when not in hard avoidance) ──────────────────────
    if (g_mode != MODE_AVOID) {
        if (radar_trigger) {
            // Radar saw something — aim camera and classify
            camera_aim(echo->bearing_deg, fix->valid ? fix->course_deg : 0.0f);
        } else {
            camera_home();
        }
        camera_update();
        handle_alert();
    }

    // ── 6. Navigation (only when avoidance is not active) ─────────────────────
    if (!avoidance_is_active() && g_mode != MODE_AVOID) {
        nav_update(fix);
    }

    // ── 7. Telemetry heartbeat ────────────────────────────────────────────────
    telemetry_heartbeat(fix, g_mode);

    // ── 8. Debug output (remove or reduce in production) ──────────────────────
    static uint32_t last_debug = 0;
    if (millis() - last_debug > 2000) {
        last_debug = millis();
        gps_print();
        radar_print();
        tof_print(&g_tof);
        Serial.print("[MAIN] Mode: ");
        const char* mode_names[] = {"STANDBY", "ACTIVE", "ALERT", "AVOID"};
        Serial.println(mode_names[(int)g_mode]);
        Serial.println("------------------------------------------");
    }
}
