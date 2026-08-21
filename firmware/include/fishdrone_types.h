/**
 * @file fishdrone_types.h
 * @brief Shared data structures used across FishDrone firmware modules.
 */

#ifndef FISHDRONE_TYPES_H
#define FISHDRONE_TYPES_H

#include <Arduino.h>

// ─────────────────────────────────────────────────────────────────────────────
// System operating mode
// ─────────────────────────────────────────────────────────────────────────────
typedef enum {
    MODE_STANDBY = 0,   ///< Low-power: only radar + TOF active
    MODE_ACTIVE,        ///< Full operation: all sensors + propulsion
    MODE_ALERT,         ///< Illegal vessel detected — transmit alert
    MODE_AVOID,         ///< Obstacle too close — override navigation
} DroneMode;

// ─────────────────────────────────────────────────────────────────────────────
// GPS fix data
// ─────────────────────────────────────────────────────────────────────────────
typedef struct {
    bool    valid;          ///< True when a valid fix is available
    float   latitude;       ///< Decimal degrees (+ = N, - = S)
    float   longitude;      ///< Decimal degrees (+ = E, - = W)
    float   speed_knots;    ///< Speed over ground
    float   course_deg;     ///< Course over ground (degrees from N)
    uint8_t satellites;     ///< Number of satellites in use
} GpsFix;

// ─────────────────────────────────────────────────────────────────────────────
// TOF distance readings (one per sensor direction)
// ─────────────────────────────────────────────────────────────────────────────
typedef struct {
    uint16_t front_mm;
    uint16_t right_mm;
    uint16_t back_mm;
    uint16_t left_mm;
    uint16_t down_mm;
} TofReadings;

// ─────────────────────────────────────────────────────────────────────────────
// Radar echo
// ─────────────────────────────────────────────────────────────────────────────
typedef struct {
    bool    detected;           ///< True when an echo is present
    float   distance_km;        ///< Estimated distance of detected target
    float   bearing_deg;        ///< Bearing of the target (degrees from N)
    uint8_t signal_strength;    ///< 0–100 (unitless, from simulator)
} RadarEcho;

// ─────────────────────────────────────────────────────────────────────────────
// Vessel classification result
// ─────────────────────────────────────────────────────────────────────────────
typedef struct {
    bool    classified;         ///< True when AI classification is available
    char    vessel_type[32];    ///< E.g. "fishing_trawler", "cargo", "unknown"
    float   confidence;         ///< 0.0–1.0
    bool    suspected_illegal;  ///< True when vessel type + zone → illegal
} VesselInfo;

// ─────────────────────────────────────────────────────────────────────────────
// Navigation command issued to the motor driver
// ─────────────────────────────────────────────────────────────────────────────
typedef struct {
    int16_t left_speed;     ///< -255 to +255 (negative = reverse)
    int16_t right_speed;    ///< -255 to +255
} MotorCommand;

// ─────────────────────────────────────────────────────────────────────────────
// Patrol waypoint
// ─────────────────────────────────────────────────────────────────────────────
typedef struct {
    float latitude;
    float longitude;
} Waypoint;

#endif // FISHDRONE_TYPES_H
