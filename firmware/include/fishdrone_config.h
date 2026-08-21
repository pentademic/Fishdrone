/**
 * @file fishdrone_config.h
 * @brief Central configuration file for FishDrone firmware.
 *
 * Edit this file to match your hardware pin assignments and tuning parameters
 * before compiling any module.
 */

#ifndef FISHDRONE_CONFIG_H
#define FISHDRONE_CONFIG_H

// ─────────────────────────────────────────────────────────────────────────────
// Serial / debug
// ─────────────────────────────────────────────────────────────────────────────
#define DEBUG_BAUD      115200   // USB Serial monitor baud rate
#define GPS_BAUD        9600     // TESEO-LIV3F default baud rate
#define RADAR_BAUD      115200   // Nucleo radar simulator baud rate
#define TELEMETRY_BAUD  9600     // Telemetry radio / UART baud rate

// ─────────────────────────────────────────────────────────────────────────────
// TOF sensor pins (VL53L1X × 5)
// Each sensor needs a dedicated XSDN (shutdown) GPIO for address reassignment.
// ─────────────────────────────────────────────────────────────────────────────
#define TOF_COUNT       5
#define TOF_XSDN_FRONT  A1
#define TOF_XSDN_RIGHT  A3
#define TOF_XSDN_BACK   A4
#define TOF_XSDN_LEFT   A5
#define TOF_XSDN_DOWN   A6

// I²C addresses assigned at boot (default is 0x29; we reassign 4 of them)
#define TOF_ADDR_FRONT  0x29
#define TOF_ADDR_RIGHT  0x2A
#define TOF_ADDR_BACK   0x2B
#define TOF_ADDR_LEFT   0x2C
#define TOF_ADDR_DOWN   0x2D

// Distance threshold (mm) below which an obstacle is considered imminent
#define TOF_OBSTACLE_THRESHOLD_MM  500

// ─────────────────────────────────────────────────────────────────────────────
// Motor driver pins
// ─────────────────────────────────────────────────────────────────────────────
#define MOTOR_LEFT_PWM  5    // PWM speed — left thruster
#define MOTOR_LEFT_DIR  6    // Direction — left thruster
#define MOTOR_RIGHT_PWM 9    // PWM speed — right thruster
#define MOTOR_RIGHT_DIR 10   // Direction — right thruster

#define MOTOR_MAX_SPEED 255  // analogWrite max
#define MOTOR_CRUISE_SPEED 150

// ─────────────────────────────────────────────────────────────────────────────
// Servo pins (camera turret)
// ─────────────────────────────────────────────────────────────────────────────
#define SERVO_PAN_PIN   7
#define SERVO_TILT_PIN  8
#define SERVO_PAN_HOME  90   // degrees
#define SERVO_TILT_HOME 45   // degrees

// ─────────────────────────────────────────────────────────────────────────────
// GPS
// ─────────────────────────────────────────────────────────────────────────────
// Connect TESEO-LIV3F TX → hardware Serial1 RX on the Nucleo/Arduino board.
// On boards with only one hardware serial, use SoftwareSerial on pins 2/3.
#define GPS_USE_SERIAL1  // comment out to use SoftwareSerial
#define GPS_SW_RX        2
#define GPS_SW_TX        3

// ─────────────────────────────────────────────────────────────────────────────
// Radar (Nucleo simulator)
// ─────────────────────────────────────────────────────────────────────────────
// Connect Nucleo TX → Serial2 RX (or SoftwareSerial on pins 4/11)
#define RADAR_USE_SERIAL2
#define RADAR_SW_RX     4
#define RADAR_SW_TX     11

// Minimum signal strength from radar to trigger active mode wake-up
#define RADAR_DETECT_THRESHOLD  10

// ─────────────────────────────────────────────────────────────────────────────
// Telemetry
// ─────────────────────────────────────────────────────────────────────────────
// Connect telemetry radio TX/RX to Serial3 (or SoftwareSerial on pins 12/13)
#define TELEM_USE_SERIAL3
#define TELEM_SW_RX     12
#define TELEM_SW_TX     13

// ─────────────────────────────────────────────────────────────────────────────
// Navigation PID tuning
// ─────────────────────────────────────────────────────────────────────────────
#define NAV_KP   1.5f
#define NAV_KI   0.02f
#define NAV_KD   0.8f

// Acceptable position error before stopping corrections (metres)
#define NAV_POSITION_TOLERANCE_M  2.0f

// ─────────────────────────────────────────────────────────────────────────────
// Power management
// ─────────────────────────────────────────────────────────────────────────────
// Time without a detection before entering standby (milliseconds)
#define POWER_STANDBY_TIMEOUT_MS  30000UL   // 30 seconds

// ─────────────────────────────────────────────────────────────────────────────
// Camera
// ─────────────────────────────────────────────────────────────────────────────
// Camera module communicates over a dedicated UART (MB1379)
// Connect MB1379 TX → Serial (or SoftwareSerial) RX
#define CAMERA_USE_SERIAL  // uses main Serial on single-serial boards
// For boards with Serial1 free, define CAMERA_USE_SERIAL1 instead

#endif // FISHDRONE_CONFIG_H
