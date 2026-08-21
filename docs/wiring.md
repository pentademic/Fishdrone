# Wiring Guide — FishDrone

## VL53L1X TOF Sensor → STM32 Nucleo Board

The VL53L1X is connected via I²C. Five sensors share the same bus; each one requires its **XSDN** (shutdown) pin connected to a separate GPIO so they can be addressed individually at startup.

### Single Sensor Pinout (VL53L1 Satellite Board)

| VL53L1 Pin | Pin Name | Nucleo Pin | Notes |
|---|---|---|---|
| 1 | INT (Interrupt) | A2 | Optional — used in interrupt mode |
| 2 | SCL_I | D15 (SCL) | Pull-up 4.7 kΩ to 3.3 V |
| 3 | XSDN_I | A1 | Shutdown pin — unique per sensor |
| 4 | SDA_I | D14 (SDA) | Pull-up 4.7 kΩ to 3.3 V |
| 5 | VDD | 3V3 | |
| 6 | GND | GND | |
| 7–10 | — | NC | Not connected |

> **Note:** Each sensor must have its own XSDN pin. Assign A1, A3, A4, A5, A6 (or similar free GPIOs) to the five sensors. Pull all XSDN lines LOW at boot, then bring them HIGH one at a time while reassigning I²C addresses to avoid conflicts.

### I²C Pull-up Resistors

Place a **4.7 kΩ** pull-up resistor between:
- SCL and 3.3 V
- SDA and 3.3 V

Only one set of pull-ups is needed for the entire bus (not one per sensor).

---

## Camera Module (MB1379) → Arduino / Nucleo

| MB1379 Pin | Connect to |
|---|---|
| VIN | 3.3 V or 5 V |
| GND | GND |
| SCL | SCL (I²C) |
| SDA | SDA (I²C) |
| TX | RX of host MCU |
| RX | TX of host MCU |
| GND | GND of host MCU |

---

## GPS Module (TESEO-LIV3F) → STM32H7

| GPS Pin | STM32 Pin | Interface |
|---|---|---|
| TX | RX (UART) | UART |
| RX | TX (UART) | UART |
| VDD | 3.3 V | Power |
| GND | GND | Ground |

---

## Servo Motors (MG996R) — Camera Turret

| Signal | STM32 Pin | Notes |
|---|---|---|
| PWM (Pan) | TIM channel (e.g. PA8) | 50 Hz, 1–2 ms pulse |
| PWM (Tilt) | TIM channel (e.g. PA9) | 50 Hz, 1–2 ms pulse |
| VCC | 5 V (external) | Servo draws high current; use dedicated supply |
| GND | Common GND | Share ground with STM32 |

---

## Propulsion Motors

Connect each DC thruster motor through a **motor driver / H-bridge** (e.g. L298N or DRV8833):

| H-Bridge Pin | STM32 Pin | Notes |
|---|---|---|
| IN1 / IN2 | GPIO + PWM | Direction + speed (PWM) |
| ENA | PWM output | Enable / speed control |
| Motor Out A/B | Thruster +/− | |
| VCC | Battery voltage | Separate from logic supply |
| GND | Common GND | |
