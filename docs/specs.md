# Hardware Specifications — FishDrone

## Microcontroller

| Parameter | Value |
|---|---|
| Board | STM32H7 (high-performance series) |
| Role | Central processing, AI inference, sensor fusion, motor control |

## Sensors

### Time-of-Flight (Obstacle Avoidance)

| Parameter | Value |
|---|---|
| Model | VL53L1X (ST VL53L1CB satellite board) |
| Quantity | 5 |
| Range | Up to 8–10 m (Long mode) |
| Interface | I²C |
| Purpose | 360° short/medium-range obstacle detection |

### Camera (Vessel Identification)

| Parameter | Value |
|---|---|
| Models | MB1683 **or** MB1379 |
| Resolution | 5 Mpx |
| Spectrum | Visible and/or Infrared |
| Purpose | AI-based boat recognition and classification |

### GPS (Position Control)

| Parameter | Value |
|---|---|
| Model | TESEO-LIV3F |
| Interface | UART / I²C |
| Purpose | Absolute position feedback for closed-loop navigation |
| Notes | Simulated in prototype phase |

### Radar (Long-range Detection)

| Parameter | Value |
|---|---|
| Type | Band-S (3 GHz) for long range; Band-X (9.41 GHz) for high resolution |
| Range | Tens of kilometres (Band-S) |
| Prototype | Simulated by a STM32 Nucleo development board |
| Purpose | Detect vessels beyond camera/TOF range in all weather conditions |

## Actuators

### Propulsion

| Parameter | Value |
|---|---|
| Type | DC underwater thruster motors |
| Quantity | 2 |
| Purpose | Directional control and speed regulation |

### Camera Turret

| Parameter | Value |
|---|---|
| Model | MG996R servo motor |
| Quantity | 2 (pan + tilt) |
| Purpose | Orient camera toward detected targets |

## Power

| Component | Role |
|---|---|
| Solar panel | Primary energy source |
| LiPo battery | Energy storage and backup |
| Standby mode | Only radar (low-power) + TOF sensors active to maximise autonomy |

## Bill of Materials (BOM) Summary

| # | Component | Model | Qty |
|---|---|---|---|
| 1 | Microcontroller | STM32H7 | 1 |
| 2 | TOF sensor | VL53L1X | 5 |
| 3 | Camera module | MB1683 / MB1379 | 1 |
| 4 | GPS module | TESEO-LIV3F | 1 |
| 5 | Servo motor | MG996R | 2 |
| 6 | Underwater thruster | DC motor | 2 |
| 7 | Radar (prototype sim) | STM32 Nucleo board | 1 |
| 8 | Solar panel | TBD | 1 |
| 9 | LiPo battery | TBD | 1 |
| 10 | Hull | Custom sheet metal | 1 |
