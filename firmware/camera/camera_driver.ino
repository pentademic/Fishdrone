/**
 * @file camera_driver.ino
 * @brief Camera interface and AI vessel classification stub — FishDrone
 *
 * Interfaces with the MB1379 (or MB1683) camera module over UART and
 * coordinates with the STM32H7 AI inference engine (STM32Cube.AI) to
 * classify detected vessels.
 *
 * In the prototype, the AI model is a placeholder that returns a fixed
 * classification based on a confidence threshold. Replace the
 * ai_run_inference() stub with the actual STM32Cube.AI generated code.
 *
 * Camera UART protocol (MB1379):
 *   - Send 'C\n'  → camera captures a JPEG and sends back a length-prefixed
 *                   binary frame: [4-byte length][JPEG bytes]
 *   - The JPEG is then passed to the AI inference engine.
 *   - For the prototype, the AI decision is simulated.
 *
 * Servo control (camera turret) uses the built-in Servo library.
 *
 * Config: firmware/include/fishdrone_config.h
 */

#include <Arduino.h>
#include <Servo.h>
#include "../include/fishdrone_config.h"
#include "../include/fishdrone_types.h"

// ─── Servo objects ────────────────────────────────────────────────────────────
static Servo _servo_pan;
static Servo _servo_tilt;

// ─── Internal state ───────────────────────────────────────────────────────────
static VesselInfo _vessel;
static bool       _capture_pending = false;
static uint32_t   _last_capture_ms = 0;
#define CAPTURE_INTERVAL_MS  2000UL   // capture a frame every 2 s when active

// ─── AI inference stub ────────────────────────────────────────────────────────
/**
 * @brief Run vessel classification on a captured image.
 *
 * In production, replace this with the output of STM32Cube.AI code generation:
 *   1. Train or convert a YOLO/MobileNet model in STM32Cube.AI.
 *   2. Generate C code (aiRun() entry point).
 *   3. Call aiRun(image_buf, output_buf) and parse output_buf here.
 *
 * @param[out] result  Populated VesselInfo struct.
 */
static void ai_run_inference(VesselInfo* result) {
    // ── Prototype stub ──
    // Simulate AI output: alternate between "fishing_trawler" and "cargo"
    static uint8_t call_count = 0;
    call_count++;

    result->classified = true;
    result->confidence = 0.82f;

    if (call_count % 3 == 0) {
        strncpy(result->vessel_type, "fishing_trawler", sizeof(result->vessel_type));
        result->suspected_illegal = true;  // assume unauthorised zone for demo
    } else if (call_count % 3 == 1) {
        strncpy(result->vessel_type, "cargo", sizeof(result->vessel_type));
        result->suspected_illegal = false;
    } else {
        strncpy(result->vessel_type, "unknown", sizeof(result->vessel_type));
        result->suspected_illegal = false;
    }
    // ── End stub ──
}

// ─── Public API ───────────────────────────────────────────────────────────────

/** Initialise servos and camera UART. Call once in setup(). */
void camera_init() {
    _servo_pan.attach(SERVO_PAN_PIN);
    _servo_tilt.attach(SERVO_TILT_PIN);
    _servo_pan.write(SERVO_PAN_HOME);
    _servo_tilt.write(SERVO_TILT_HOME);

    memset(&_vessel, 0, sizeof(_vessel));
    Serial.println("[CAM] Camera driver ready.");
}

/**
 * @brief Point the camera turret at a target bearing (0–360°).
 * Tilt angle is fixed at SERVO_TILT_HOME for horizon-level aiming.
 *
 * @param bearing_deg  Absolute bearing of the target (degrees from North).
 * @param own_course   Current drone course (degrees from North).
 */
void camera_aim(float bearing_deg, float own_course) {
    // Compute relative angle: how far left/right of the drone's bow
    float relative = bearing_deg - own_course;
    if (relative >  180.0f) relative -= 360.0f;
    if (relative < -180.0f) relative += 360.0f;

    // Map to servo range: 0° = full left, 90° = centre, 180° = full right
    int pan_angle = (int)constrain(relative + 90.0f, 0.0f, 180.0f);
    _servo_pan.write(pan_angle);
    _servo_tilt.write(SERVO_TILT_HOME);
}

/** Return the camera to the forward-facing home position. */
void camera_home() {
    _servo_pan.write(SERVO_PAN_HOME);
    _servo_tilt.write(SERVO_TILT_HOME);
}

/**
 * @brief Non-blocking capture + classify — call every loop iteration when active.
 *
 * Triggers a new capture every CAPTURE_INTERVAL_MS ms, runs AI inference,
 * and stores the result internally.
 */
void camera_update() {
    if ((millis() - _last_capture_ms) < CAPTURE_INTERVAL_MS) return;
    _last_capture_ms = millis();

    // In production: send capture command to camera module and wait for JPEG.
    // Here we skip actual UART transfer and go straight to inference stub.
    ai_run_inference(&_vessel);

    if (_vessel.classified) {
        Serial.print("[CAM] Vessel: ");
        Serial.print(_vessel.vessel_type);
        Serial.print(" (conf=");
        Serial.print(_vessel.confidence, 2);
        Serial.print(") illegal=");
        Serial.println(_vessel.suspected_illegal ? "YES" : "no");
    }
}

/** Return the latest vessel classification (read-only). */
const VesselInfo* camera_get_vessel() {
    return &_vessel;
}

/** Clear the last classification result. */
void camera_clear_vessel() {
    memset(&_vessel, 0, sizeof(_vessel));
}

// ─── Standalone test sketch ───────────────────────────────────────────────────

void setup() {
    Serial.begin(DEBUG_BAUD);
    while (!Serial) {}
    camera_init();
    Serial.println("[CAM] Test: classifying frames every 2 s...");
}

void loop() {
    camera_update();

    // Sweep pan servo back and forth for visual test
    static int  pan = 0;
    static int  dir = 1;
    pan += dir * 5;
    if (pan >= 180 || pan <= 0) dir = -dir;
    _servo_pan.write(pan);

    delay(100);
}
