/**
 * @file avoidance.ino
 * @brief Obstacle avoidance logic — FishDrone
 *
 * Monitors the five TOF sensors and overrides the navigation controller when
 * an obstacle is detected within the configured threshold.
 *
 * Avoidance strategy:
 *   - FRONT obstacle → back up briefly, then turn toward the side with more
 *     clearance (right vs. left), then resume navigation.
 *   - SIDE obstacle  → steer away from the obstructed side.
 *   - BACK obstacle  → stop reversing (if the avoidance routine was backing up).
 *
 * The avoidance module owns the motors while AVOID mode is active; the
 * navigation module resumes control once the path is clear.
 *
 * Threshold and pin config: firmware/include/fishdrone_config.h
 */

#include <Arduino.h>
#include "../include/fishdrone_config.h"
#include "../include/fishdrone_types.h"

// ─── Internal state ───────────────────────────────────────────────────────────

typedef enum {
    AV_CLEAR = 0,     ///< No obstacle — navigation in control
    AV_BACKING,       ///< Reversing away from front obstacle
    AV_TURNING,       ///< Turning to clear path
    AV_WAITING,       ///< Short pause before resuming
} AvoidState;

static AvoidState _state       = AV_CLEAR;
static uint32_t   _state_enter = 0;

// Durations for each phase (ms)
#define AV_BACK_DURATION_MS   800
#define AV_TURN_DURATION_MS   1200
#define AV_WAIT_DURATION_MS   300

// ─── Forward declarations (nav module) ───────────────────────────────────────
extern void nav_stop();
extern void nav_apply_command(const MotorCommand*);

// ─── Internal helpers ─────────────────────────────────────────────────────────

static void enter_state(AvoidState next) {
    _state       = next;
    _state_enter = millis();
}

static bool state_elapsed(uint32_t duration_ms) {
    return (millis() - _state_enter) >= duration_ms;
}

// ─── Public API ───────────────────────────────────────────────────────────────

/**
 * @brief Non-blocking avoidance update. Call every loop iteration.
 *
 * @param[in]  readings  Latest TOF distance readings.
 * @param[out] mode      Pointer to the global DroneMode; set to MODE_AVOID
 *                       while an obstacle is being handled.
 */
void avoidance_update(const TofReadings* readings, DroneMode* mode) {
    MotorCommand cmd;

    switch (_state) {

    case AV_CLEAR:
        if (readings->front_mm < TOF_OBSTACLE_THRESHOLD_MM) {
            Serial.print("[AVOID] Front obstacle at ");
            Serial.print(readings->front_mm);
            Serial.println(" mm — backing up");
            *mode = MODE_AVOID;
            enter_state(AV_BACKING);
        } else if (readings->left_mm < TOF_OBSTACLE_THRESHOLD_MM) {
            Serial.println("[AVOID] Left obstacle — steering right");
            *mode = MODE_AVOID;
            // Immediate single-step correction: turn right
            cmd.left_speed  = MOTOR_CRUISE_SPEED;
            cmd.right_speed = MOTOR_CRUISE_SPEED / 2;
            nav_apply_command(&cmd);
            // Stay in CLEAR — recovers naturally once side clears
        } else if (readings->right_mm < TOF_OBSTACLE_THRESHOLD_MM) {
            Serial.println("[AVOID] Right obstacle — steering left");
            *mode = MODE_AVOID;
            cmd.left_speed  = MOTOR_CRUISE_SPEED / 2;
            cmd.right_speed = MOTOR_CRUISE_SPEED;
            nav_apply_command(&cmd);
        } else {
            *mode = MODE_ACTIVE;  // path clear, return control to nav
        }
        break;

    case AV_BACKING:
        // Reverse both motors
        cmd.left_speed  = -(int16_t)(MOTOR_CRUISE_SPEED * 0.7f);
        cmd.right_speed = -(int16_t)(MOTOR_CRUISE_SPEED * 0.7f);
        nav_apply_command(&cmd);

        if (state_elapsed(AV_BACK_DURATION_MS)) {
            // Choose turn direction: go toward side with more clearance
            Serial.println("[AVOID] Backed up — turning");
            enter_state(AV_TURNING);
        }
        break;

    case AV_TURNING: {
        bool turn_right = (readings->right_mm >= readings->left_mm);
        if (turn_right) {
            cmd.left_speed  =  MOTOR_CRUISE_SPEED;
            cmd.right_speed = -MOTOR_CRUISE_SPEED;
        } else {
            cmd.left_speed  = -MOTOR_CRUISE_SPEED;
            cmd.right_speed =  MOTOR_CRUISE_SPEED;
        }
        nav_apply_command(&cmd);

        if (state_elapsed(AV_TURN_DURATION_MS)) {
            Serial.println("[AVOID] Turn complete — waiting");
            nav_stop();
            enter_state(AV_WAITING);
        }
        break;
    }

    case AV_WAITING:
        if (state_elapsed(AV_WAIT_DURATION_MS)) {
            // Re-evaluate: if front is now clear, return to navigation
            if (readings->front_mm >= TOF_OBSTACLE_THRESHOLD_MM) {
                Serial.println("[AVOID] Path clear — resuming navigation");
                *mode = MODE_ACTIVE;
                enter_state(AV_CLEAR);
            } else {
                // Still blocked — try backing up again
                Serial.println("[AVOID] Still blocked — retry");
                enter_state(AV_BACKING);
            }
        }
        break;
    }
}

/**
 * @brief Return true while the avoidance module has overridden navigation.
 */
bool avoidance_is_active() {
    return (_state != AV_CLEAR);
}

// ─── Standalone test sketch ───────────────────────────────────────────────────

extern bool        tof_init();
extern void        tof_read_all(TofReadings*);
extern void        tof_print(const TofReadings*);
extern void        nav_init();

void setup() {
    Serial.begin(DEBUG_BAUD);
    while (!Serial) {}
    if (!tof_init()) {
        Serial.println("[AVOID] TOF init failed.");
        while (1) {}
    }
    nav_init();
    Serial.println("[AVOID] Avoidance system ready.");
}

void loop() {
    TofReadings readings;
    tof_read_all(&readings);
    tof_print(&readings);

    DroneMode mode = MODE_ACTIVE;
    avoidance_update(&readings, &mode);

    delay(50);
}
