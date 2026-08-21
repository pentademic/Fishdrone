# Firmware — FishDrone

This directory contains the complete embedded firmware source code for the FishDrone platform (STM32H7 / Nucleo).

---

## Directory Layout

```
firmware/
├── README.md                          ← This file
├── include/
│   ├── fishdrone_config.h             ← Pin assignments, baud rates, PID tuning
│   └── fishdrone_types.h              ← Shared data structures (GpsFix, TofReadings, …)
│
├── main/
│   └── fishdrone_main.ino             ← Top-level integration sketch (start here)
│
├── tof/
│   ├── tof_sensor.ino                 ← Single VL53L1X sensor (standalone test)
│   └── tof_multi.ino                  ← 5-sensor manager with address reassignment
│
├── gps/
│   └── gps_driver.ino                 ← NMEA parser for TESEO-LIV3F
│
├── radar/
│   └── radar_driver.ino               ← UART interface to Nucleo radar simulator
│                                         (also contains the Nucleo simulator sketch)
├── navigation/
│   └── navigation.ino                 ← PID waypoint controller + motor driver
│
├── avoidance/
│   └── avoidance.ino                  ← TOF-based obstacle avoidance state machine
│
├── power/
│   └── power_mgmt.ino                 ← Sleep / wake state machine
│
├── telemetry/
│   └── telemetry.ino                  ← Alert TX + command RX over radio UART
│
└── camera/
    └── camera_driver.ino              ← Servo turret + camera UART + AI stub
```

---

## Quick Start — Full System Build

### Step 1 — Prepare a single Arduino sketch folder

Arduino IDE requires all `.ino` files in the same directory as the main sketch.
Copy or symlink all module files into one folder:

```bash
mkdir fishdrone_main_build
cp firmware/include/*.h          fishdrone_main_build/
cp firmware/main/fishdrone_main.ino  fishdrone_main_build/
cp firmware/tof/tof_multi.ino    fishdrone_main_build/
cp firmware/gps/gps_driver.ino   fishdrone_main_build/
cp firmware/radar/radar_driver.ino fishdrone_main_build/
cp firmware/navigation/navigation.ino fishdrone_main_build/
cp firmware/avoidance/avoidance.ino   fishdrone_main_build/
cp firmware/power/power_mgmt.ino      fishdrone_main_build/
cp firmware/telemetry/telemetry.ino   fishdrone_main_build/
cp firmware/camera/camera_driver.ino  fishdrone_main_build/
```

Then open `fishdrone_main_build/fishdrone_main.ino` in Arduino IDE.

### Step 2 — Install Arduino IDE with STM32duino

1. Download [Arduino IDE 2.x](https://www.arduino.cc/en/software)
2. Open *File → Preferences*, add to "Additional boards manager URLs":
   ```
   https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
   ```
3. Open *Tools → Board → Boards Manager*, search **STM32** and install.

### Step 3 — Install required libraries

Via *Sketch → Include Library → Manage Libraries*:

| Library | Publisher | Used by |
|---|---|---|
| `VL53L1X` | Pololu | tof_sensor, tof_multi |
| `Wire` | Built-in | tof drivers |
| `Servo` | Built-in | camera_driver |
| `SoftwareSerial` | Built-in | GPS / Radar / Telemetry (if no hardware UART free) |

### Step 4 — Configure pin assignments

Edit `fishdrone_config.h` to match your board wiring **before compiling**. Key settings:

```c
#define TOF_XSDN_FRONT  A1   // XSDN GPIO for front TOF sensor
#define MOTOR_LEFT_PWM  5    // PWM pin for left thruster
#define GPS_USE_SERIAL1      // Use hardware Serial1 for GPS
// ... see full file for all options
```

### Step 5 — Select board and upload

- *Tools → Board → STM32 boards → Nucleo-64* (adjust to your exact Nucleo variant)
- Select the correct COM port
- Click **Upload**

---

## Individual Module Tests

Each module contains its own `setup()` / `loop()` for standalone testing.
To test a single module, open only that file in Arduino IDE (e.g. `tof_multi.ino`)
and upload it directly. All modules print to **Serial at 115200 baud**.

| Module | Test output |
|---|---|
| `tof_sensor.ino` | `Distance: 342 mm` |
| `tof_multi.ino` | `[TOF] F:342 R:1200 B:800 L:950 D:120 mm` |
| `gps_driver.ino` | `[GPS] Lat:43.296500 Lon:5.381300 …` |
| `radar_driver.ino` | `[RADAR] Vessel @ 3.45 km, bearing 127°, signal 72` |
| `navigation.ino` | `[NAV] Route loaded: 3 waypoints.` |
| `avoidance.ino` | `[AVOID] Front obstacle at 320 mm — backing up` |
| `power_mgmt.ino` | `[PWR] Mode: STANDBY` |
| `telemetry.ino` | `[TELEM] Heartbeat: HBEAT,43.296500,5.381300,ACTIVE` |
| `camera_driver.ino` | `[CAM] Vessel: fishing_trawler (conf=0.82) illegal=YES` |

---

## Radar Simulator (Nucleo board)

Flash `radar/radar_driver.ino` to the **second Nucleo board** with `RADAR_SIMULATOR` uncommented at the top of the file. Connect its TX pin to the main board's radar RX pin. It will emit simulated UART radar frames every 500 ms.

---

## Option B — STM32CubeIDE (production build)

1. Install [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html)
2. Create a new STM32 project targeting your H7 part number (e.g. STM32H743ZI).
3. Configure peripherals in STM32CubeMX:

   | Peripheral | Function |
   |---|---|
   | I²C1 | 5× VL53L1X TOF sensors |
   | USART1 | GPS (TESEO-LIV3F) |
   | USART2 | Radar (Nucleo simulator) |
   | USART3 | Telemetry radio |
   | TIM1 CH1/CH2 | Servo PWM (pan/tilt) |
   | TIM3 CH1/CH2 | Motor PWM (left/right thruster) |
   | GPIO outputs | Motor DIR pins, TOF XSDN pins |

4. Generate code, add the `.ino` files to the project (compile as C++), and resolve `Serial` → `printf` / HAL_UART aliases.
5. Flash via ST-LINK.

---

## Flashing via ST-LINK CLI

```bash
# Requires STM32CubeProgrammer installed
STM32_Programmer_CLI -c port=SWD -w build/fishdrone.bin 0x08000000 -v -rst
```

---

## AI Integration (STM32Cube.AI)

The `camera/camera_driver.ino` contains a stub `ai_run_inference()` function.
To replace it with a real model:

1. Train or convert a vessel-classification model (e.g. MobileNetV2 / YOLOv5-nano)
   to TensorFlow Lite or ONNX format.
2. Open STM32CubeIDE → *Tools → STM32Cube.AI*, import the model.
3. Generate the C inference code (`aiRun()` entry point).
4. Replace the stub body in `camera_driver.ino` with a call to `aiRun()` and
   parse the output tensor to populate the `VesselInfo` struct.
