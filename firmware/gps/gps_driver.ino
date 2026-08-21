/**
 * @file gps_driver.ino
 * @brief GPS driver for TESEO-LIV3F — FishDrone position sensing
 *
 * Parses NMEA 0183 sentences from the TESEO-LIV3F GPS module over UART and
 * exposes the current fix via the GpsFix struct (see fishdrone_types.h).
 *
 * Supported sentences:
 *   $GNGGA — fix quality, latitude, longitude, satellite count
 *   $GNRMC — speed, course, validity flag
 *
 * Wiring (see docs/wiring.md):
 *   TESEO TX → Serial1 RX (pin 0 on most Nucleo boards)
 *   TESEO RX → Serial1 TX (pin 1) — optional, only needed to send commands
 *   TESEO VDD → 3.3 V
 *   TESEO GND → GND
 *
 * Pin / baud configuration: firmware/include/fishdrone_config.h
 *
 * Libraries: none required (raw NMEA parsing).
 */

#include "../include/fishdrone_config.h"
#include "../include/fishdrone_types.h"

#ifndef GPS_USE_SERIAL1
#include <SoftwareSerial.h>
static SoftwareSerial gpsSerial(GPS_SW_RX, GPS_SW_TX);
#define GPS_SERIAL gpsSerial
#else
#define GPS_SERIAL Serial1
#endif

// ─── Internal state ───────────────────────────────────────────────────────────
static GpsFix  _fix;
static char    _buf[128];
static uint8_t _buf_idx = 0;

// ─── NMEA helpers ─────────────────────────────────────────────────────────────

/** Extract the Nth comma-separated field from an NMEA sentence into out[]. */
static void nmea_field(const char* sentence, uint8_t field_num, char* out, uint8_t out_size) {
    uint8_t field = 0;
    uint8_t out_i = 0;
    for (uint8_t i = 0; sentence[i] != '\0' && out_i < out_size - 1; i++) {
        if (sentence[i] == ',') {
            field++;
            if (field > field_num) break;
        } else if (field == field_num) {
            out[out_i++] = sentence[i];
        }
    }
    out[out_i] = '\0';
}

/** Convert NMEA ddmm.mmmm + hemisphere char to decimal degrees. */
static float nmea_coord_to_decimal(const char* coord, char hemi) {
    if (coord[0] == '\0') return 0.0f;
    float raw = atof(coord);
    int   deg = (int)(raw / 100);
    float min = raw - (deg * 100.0f);
    float decimal = deg + min / 60.0f;
    if (hemi == 'S' || hemi == 'W') decimal = -decimal;
    return decimal;
}

/** Verify NMEA checksum. Returns true if valid. */
static bool nmea_checksum_valid(const char* sentence) {
    if (sentence[0] != '$') return false;
    uint8_t calc = 0;
    uint8_t i = 1;
    while (sentence[i] != '*' && sentence[i] != '\0') {
        calc ^= (uint8_t)sentence[i++];
    }
    if (sentence[i] != '*') return false;
    uint8_t recv = (uint8_t)strtol(&sentence[i + 1], nullptr, 16);
    return calc == recv;
}

// ─── Sentence parsers ─────────────────────────────────────────────────────────

/** Parse $GNGGA: lat, lon, fix quality, satellite count. */
static void parse_GGA(const char* s) {
    char field[16];

    nmea_field(s, 2, field, sizeof(field));  // latitude
    char lat_str[16];
    memcpy(lat_str, field, sizeof(lat_str));

    nmea_field(s, 3, field, sizeof(field));  // N/S
    char lat_hemi = field[0];

    nmea_field(s, 4, field, sizeof(field));  // longitude
    char lon_str[16];
    memcpy(lon_str, field, sizeof(lon_str));

    nmea_field(s, 5, field, sizeof(field));  // E/W
    char lon_hemi = field[0];

    nmea_field(s, 6, field, sizeof(field));  // fix quality (0=invalid)
    uint8_t quality = (uint8_t)atoi(field);

    nmea_field(s, 7, field, sizeof(field));  // satellites
    _fix.satellites = (uint8_t)atoi(field);

    _fix.valid     = (quality > 0);
    _fix.latitude  = nmea_coord_to_decimal(lat_str, lat_hemi);
    _fix.longitude = nmea_coord_to_decimal(lon_str, lon_hemi);
}

/** Parse $GNRMC: speed, course, validity. */
static void parse_RMC(const char* s) {
    char field[16];

    nmea_field(s, 2, field, sizeof(field));  // status A=active V=void
    _fix.valid = (field[0] == 'A');

    nmea_field(s, 7, field, sizeof(field));  // speed over ground (knots)
    _fix.speed_knots = atof(field);

    nmea_field(s, 8, field, sizeof(field));  // course over ground
    _fix.course_deg = atof(field);
}

/** Dispatch a complete NMEA sentence to the correct parser. */
static void dispatch_sentence(const char* sentence) {
    if (!nmea_checksum_valid(sentence)) return;

    if      (strncmp(sentence + 3, "GGA", 3) == 0) parse_GGA(sentence);
    else if (strncmp(sentence + 3, "RMC", 3) == 0) parse_RMC(sentence);
}

// ─── Public API ───────────────────────────────────────────────────────────────

/** Initialise the GPS UART. Call once in setup(). */
void gps_init() {
    GPS_SERIAL.begin(GPS_BAUD);
    memset(&_fix, 0, sizeof(_fix));
    Serial.println("[GPS] Driver ready.");
}

/**
 * @brief Non-blocking update — call every loop iteration.
 *
 * Reads available bytes from the GPS UART, buffers them, and parses complete
 * NMEA sentences. Does not block.
 */
void gps_update() {
    while (GPS_SERIAL.available()) {
        char c = (char)GPS_SERIAL.read();
        if (c == '\n' || c == '\r') {
            if (_buf_idx > 0) {
                _buf[_buf_idx] = '\0';
                dispatch_sentence(_buf);
                _buf_idx = 0;
            }
        } else if (_buf_idx < sizeof(_buf) - 1) {
            _buf[_buf_idx++] = c;
        } else {
            // buffer overflow — discard this sentence
            _buf_idx = 0;
        }
    }
}

/** Return a pointer to the latest GPS fix (read-only). */
const GpsFix* gps_get_fix() {
    return &_fix;
}

/**
 * @brief Compute great-circle bearing from current position to a waypoint.
 * @return Bearing in degrees (0–360), or 0 if fix is invalid.
 */
float gps_bearing_to(float target_lat, float target_lon) {
    if (!_fix.valid) return 0.0f;
    float dlat = radians(target_lat - _fix.latitude);
    float dlon = radians(target_lon - _fix.longitude);
    float lat1 = radians(_fix.latitude);
    float lat2 = radians(target_lat);

    float x = sin(dlon) * cos(lat2);
    float y = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dlon);
    float bearing = degrees(atan2(x, y));
    return fmod(bearing + 360.0f, 360.0f);
}

/**
 * @brief Compute approximate distance (metres) from current position to target.
 * Uses the Haversine formula. Returns 0 if fix is invalid.
 */
float gps_distance_to_m(float target_lat, float target_lon) {
    if (!_fix.valid) return 0.0f;
    const float R = 6371000.0f;  // Earth radius in metres
    float phi1 = radians(_fix.latitude);
    float phi2 = radians(target_lat);
    float dphi = radians(target_lat - _fix.latitude);
    float dlam  = radians(target_lon - _fix.longitude);

    float a = sin(dphi / 2) * sin(dphi / 2) +
              cos(phi1) * cos(phi2) * sin(dlam / 2) * sin(dlam / 2);
    return R * 2.0f * atan2(sqrt(a), sqrt(1.0f - a));
}

/** Print current fix to Serial (debug). */
void gps_print() {
    if (_fix.valid) {
        Serial.print("[GPS] Lat:");    Serial.print(_fix.latitude, 6);
        Serial.print(" Lon:");         Serial.print(_fix.longitude, 6);
        Serial.print(" Spd:");         Serial.print(_fix.speed_knots);
        Serial.print("kn Crs:");       Serial.print(_fix.course_deg);
        Serial.print("° Sats:");       Serial.println(_fix.satellites);
    } else {
        Serial.println("[GPS] No fix.");
    }
}

// ─── Standalone test sketch ───────────────────────────────────────────────────

void setup() {
    Serial.begin(DEBUG_BAUD);
    while (!Serial) {}
    gps_init();
}

void loop() {
    gps_update();
    gps_print();
    delay(1000);
}
