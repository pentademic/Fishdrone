# System Architecture — FishDrone

## High-Level Overview

```
┌──────────────────────────────────────────────────────────────────┐
│                        FishDrone Platform                        │
│                                                                  │
│  ┌───────────┐   ┌───────────┐   ┌───────────┐   ┌──────────┐  │
│  │  Radar    │   │ Camera +  │   │    GPS    │   │  5× TOF  │  │
│  │(Nucleo    │   │  AI (5Mpx)│   │TESEO-LIV3F│   │ VL53L1X  │  │
│  │ simulated)│   │           │   │           │   │          │  │
│  └─────┬─────┘   └─────┬─────┘   └─────┬─────┘   └────┬─────┘  │
│        │               │               │               │        │
│        └───────────────┴───────────────┴───────────────┘        │
│                                │                                 │
│                   ┌────────────▼────────────┐                   │
│                   │       STM32H7           │                   │
│                   │  (Central Controller)   │                   │
│                   │  - Sensor fusion        │                   │
│                   │  - AI inference         │                   │
│                   │  - Navigation logic     │                   │
│                   │  - Power management     │                   │
│                   └────────────┬────────────┘                   │
│                                │                                 │
│            ┌───────────────────┼───────────────────┐            │
│            │                   │                   │            │
│     ┌──────▼──────┐   ┌────────▼────────┐  ┌──────▼──────┐    │
│     │  2× Thruster│   │  2× MG996R Servo│  │  Comms / TX │    │
│     │   Motors    │   │  (Camera turret)│  │  (telemetry)│    │
│     └─────────────┘   └─────────────────┘  └─────────────┘    │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

---

## Operating Modes

### Normal (Active) Mode

All systems are active:

1. **Radar** continuously scans for vessels at long range (kilometres).
2. **Camera** + AI identifies detected vessels and checks registration/legality.
3. **GPS** provides absolute position; the navigation controller keeps the drone on its patrol path.
4. **TOF sensors** (×5) monitor the surrounding 360° for close obstacles and trigger avoidance manoeuvres.
5. **Propulsion** adjusts heading and speed based on navigation and avoidance commands.
6. **Servo turret** aims the camera toward detected targets.

### Standby (Low-Power) Mode

Activated automatically to preserve battery when no threat is detected:

- Radar is kept running in **low-power scan mode**.
- All TOF sensors remain active but in **waiting state**.
- Camera, AI, GPS, and propulsion are **suspended**.
- On radar or TOF trigger → system wakes to Normal mode within seconds.

---

## Software Modules (Planned)

| Module | Responsibility | Interface |
|---|---|---|
| `radar_driver` | Parse radar data from Nucleo UART | UART |
| `tof_driver` | Poll/interrupt VL53L1X sensors | I²C |
| `gps_driver` | Parse NMEA sentences from TESEO | UART |
| `camera_driver` | Frame capture + UART to camera module | I²C / UART |
| `ai_inference` | Run boat-detection model (STM32 AI) | Internal |
| `navigation` | PID position controller (GPS + propulsion) | PWM / GPIO |
| `avoidance` | React to TOF readings, override navigation | Internal |
| `power_mgmt` | Manage sleep/wake transitions | GPIO / RCC |
| `telemetry` | Transmit alerts and position data | UART / RF |

---

## Data Flow — Vessel Detection

```
Radar detects echo
        │
        ▼
STM32H7 wakes camera & AI
        │
        ▼
Camera captures frame → AI classifies vessel type
        │
        ▼
Is vessel authorised in this zone?
   ├── YES → log, continue patrol
   └── NO  → log alert, transmit GPS coords + image via telemetry
```

---

## Power Budget (Estimates)

| Component | Typical Current |
|---|---|
| STM32H7 (full speed) | ~100–300 mA |
| Camera | ~100 mA |
| 5× VL53L1X | ~5 mA total |
| GPS | ~20 mA |
| 2× Servo (idle) | ~10 mA |
| 2× Thruster (cruise) | ~2–5 A |
| Radar (active) | ~500 mA–1 A |

Solar panel and battery capacity should be sized to sustain continuous operation in the target patrol area.
