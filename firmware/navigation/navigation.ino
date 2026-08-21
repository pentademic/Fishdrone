/**
 * @file navigation.ino
 * @brief GPS-based navigation and PID position controller — FishDrone
 *
 * Controls the two thruster motors to steer FishDrone along a sequence of
 * GPS waypoints using a heading PID controller.
 *
 * Algorithm:
 *   1. Compute bearing from current GPS fix to the next waypoint.
 *   2. Compute heading error: error = target_bearing - current_heading.
 *      (Normalise to [-180, +180] to handle the 0/360 wraparound.)
 *   3. Run PID on the heading error to compute a differential correction.
 *   4. Apply: left_speed = base + correction, right_speed = base - correction.
 *   5. When within NAV_POSITION_TOLERANCE_M of the waypoint, advance to next.
 *
 * Motor driver assumed: L298N or DRV8833-style H-bridge.
 *   PWM pin  → analogWrite(speed)
 *   DIR pin  → digitalWrite(HIGH=forward, LOW=reverse)
 *
 * Pin / tuning: firmware/include/fishdrone_config.h
 */

#include <Arduino.h>
#include "../include/fishdrone_config.h"
#include "../include/fishdrone_types.h"

// ─── Internal state ───────────────────────────────────────────────────────────

static Waypoint _waypoints[16];
static uint8_t  _waypoint_count = 0;
static uint8_t  _current_wp     = 0;

// PID state
static float    _integral     = 0.0f;
static float    _prev_error   = 0.0f;
static uint32_t _last_pid_ms  = 0;

// ─── Motor driver helpers ─────────────────────────────────────────────────────

static void motor_set(uint8_t pwm_pin, uint8_t dir_pin, int16_t speed) {
    if (speed >= 0) {
        digitalWrite(dir_pin, HIGH);
        analogWrite(pwm_pin, (uint8_t)constrain(speed, 0, MOTOR_MAX_SPEED));
    } else {
        digitalWrite(dir_pin, LOW);
        analogWrite(pwm_pin, (uint8_t)constrain(-speed, 0, MOTOR_MAX_SPEED));
    }
}

// ─── Public API ───────────────────────────────────────────────────────────────

/** Initialise motor driver pins. Call once in setup(). */
void nav_init() {
    pinMode(MOTOR_LEFT_PWM,  OUTPUT);
    pinMode(MOTOR_LEFT_DIR,  OUTPUT);
    pinMode(MOTOR_RIGHT_PWM, OUTPUT);
    pinMode(MOTOR_RIGHT_DIR, OUTPUT);

    // Ensure motors are stopped
    analogWrite(MOTOR_LEFT_PWM,  0);
    analogWrite(MOTOR_RIGHT_PWM, 0);

    _last_pid_ms = millis();
    Serial.println("[NAV] Motor driver ready.");
}

/**
 * @brief Load a patrol route.
 * @param wps    Array of Waypoint structs.
 * @param count  Number of waypoints (max 16).
 */
void nav_load_route(const Waypoint* wps, uint8_t count) {
    _waypoint_count = min(count, (uint8_t)16);
    memcpy(_waypoints, wps, _waypoint_count * sizeof(Waypoint));
    _current_wp = 0;
    Serial.print("[NAV] Route loaded: ");
    Serial.print(_waypoint_count);
    Serial.println(" waypoints.");
}

/**
 * @brief Stop both motors immediately.
 */
void nav_stop() {
    analogWrite(MOTOR_LEFT_PWM,  0);
    analogWrite(MOTOR_RIGHT_PWM, 0);
    _integral   = 0.0f;
    _prev_error = 0.0f;
}

/**
 * @brief Apply an explicit motor command (used by avoidance module).
 */
void nav_apply_command(const MotorCommand* cmd) {
    motor_set(MOTOR_LEFT_PWM,  MOTOR_LEFT_DIR,  cmd->left_speed);
    motor_set(MOTOR_RIGHT_PWM, MOTOR_RIGHT_DIR, cmd->right_speed);
}

/**
 * @brief Non-blocking navigation update — call every loop iteration.
 *
 * Reads the current GPS fix (via gps_get_fix()), computes the heading PID
 * correction, and drives the motors. Does nothing if no fix or no route.
 *
 * @param[in] fix  Current GPS fix from gps_get_fix().
 */
void nav_update(const GpsFix* fix) {
    if (_waypoint_count == 0) return;
    if (!fix->valid) {
        Serial.println("[NAV] Waiting for GPS fix...");
        return;
    }

    Waypoint* wp = &_waypoints[_current_wp];

    // Check arrival
    // We need the gps_distance_to_m helper from gps_driver — declared extern here.
    extern float gps_distance_to_m(float, float);
    float dist_m = gps_distance_to_m(wp->latitude, wp->longitude);
    if (dist_m < NAV_POSITION_TOLERANCE_M) {
        Serial.print("[NAV] Reached waypoint ");
        Serial.println(_current_wp);
        _current_wp = (_current_wp + 1) % _waypoint_count;
        _integral   = 0.0f;
        _prev_error = 0.0f;
        return;
    }

    // Target bearing
    extern float gps_bearing_to(float, float);
    float target_bearing = gps_bearing_to(wp->latitude, wp->longitude);

    // Heading error (normalise to [-180, +180])
    float error = target_bearing - fix->course_deg;
    if (error >  180.0f) error -= 360.0f;
    if (error < -180.0f) error += 360.0f;

    // PID timestep
    uint32_t now = millis();
    float dt = (now - _last_pid_ms) / 1000.0f;
    if (dt <= 0.0f) dt = 0.033f;
    _last_pid_ms = now;

    _integral   += error * dt;
    _integral    = constrain(_integral, -100.0f, 100.0f);  // anti-windup

    float derivative = (error - _prev_error) / dt;
    _prev_error = error;

    float correction = NAV_KP * error + NAV_KI * _integral + NAV_KD * derivative;
    correction = constrain(correction, -(float)MOTOR_MAX_SPEED, (float)MOTOR_MAX_SPEED);

    MotorCommand cmd;
    cmd.left_speed  = (int16_t)constrain(MOTOR_CRUISE_SPEED + correction,
                                          -MOTOR_MAX_SPEED, MOTOR_MAX_SPEED);
    cmd.right_speed = (int16_t)constrain(MOTOR_CRUISE_SPEED - correction,
                                          -MOTOR_MAX_SPEED, MOTOR_MAX_SPEED);
    nav_apply_command(&cmd);
}

// ─── Standalone test sketch ───────────────────────────────────────────────────

// Declare external GPS functions (provided by gps_driver.ino in full build)
extern void       gps_init();
extern void       gps_update();
extern const GpsFix* gps_get_fix();

void setup() {
    Serial.begin(DEBUG_BAUD);
    while (!Serial) {}
    gps_init();
    nav_init();

    // Example patrol route (replace with real coordinates)
    Waypoint route[] = {
        {43.2965f,  5.3813f},  // Marseille harbour entrance
        {43.2900f,  5.3700f},  // Patrol point A
        {43.2850f,  5.3900f},  // Patrol point B
    };
    nav_load_route(route, 3);
}

void loop() {
    gps_update();
    const GpsFix* fix = gps_get_fix();
    nav_update(fix);
    delay(100);
}
