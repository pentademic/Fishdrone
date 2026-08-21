# Firmware — FishDrone

This directory contains the embedded firmware source code for the FishDrone platform (STM32H7).

---

## Directory Layout

```
firmware/
└── tof/
    └── tof_sensor.ino   ← VL53L1X TOF sensor driver (Arduino/STM32duino)
```

---

## Build Options

### Option A — Arduino IDE with STM32duino

Best for rapid prototyping on a Nucleo board.

1. **Install Arduino IDE** — https://www.arduino.cc/en/software (v2.x recommended)
2. **Add STM32 board package:**
   - Open *File → Preferences*
   - Add to "Additional boards manager URLs":
     ```
     https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
     ```
   - Open *Tools → Board → Boards Manager*, search **STM32** and install.
3. **Install required libraries** via *Sketch → Include Library → Manage Libraries*:
   - `VL53L1X` by Pololu
   - `Wire` (built-in)
4. **Select board:** *Tools → Board → STM32 boards → Nucleo-64* (or the matching Nucleo variant)
5. Open `firmware/tof/tof_sensor.ino`, select the correct COM port, and click **Upload**.

### Option B — STM32CubeIDE

Best for production-grade development on the STM32H7.

1. **Install STM32CubeIDE** — https://www.st.com/en/development-tools/stm32cubeide.html
2. Create a new STM32 project targeting your H7 part number.
3. Use STM32CubeMX to configure peripherals:
   - I²C1 → TOF sensors (VL53L1X)
   - USART1 → GPS (TESEO-LIV3F), debug
   - USART2 → Radar (Nucleo simulation)
   - TIM1/TIM2 → PWM for servo motors
   - GPIO outputs → Motor driver IN pins, sensor XSDN pins
4. Generate code, then integrate the sensor drivers into the generated project.
5. Flash via ST-LINK (USB cable or header pins on the Nucleo board).

---

## Flashing via ST-LINK CLI (alternative)

```bash
# Install STM32CubeProgrammer, then:
STM32_Programmer_CLI -c port=SWD -w firmware.bin 0x08000000 -v -rst
```

---

## Serial Monitor

After flashing `tof_sensor.ino`, open the Serial Monitor at **115200 baud** to see live distance readings:

```
Distance: 342 mm
Distance: 345 mm
Distance: 1024 mm
...
```

---

## Next Steps

The following firmware modules still need to be implemented:

- [ ] Multi-sensor TOF manager (address assignment for 5× VL53L1X)
- [ ] GPS driver (NMEA parser for TESEO-LIV3F)
- [ ] Radar interface (UART protocol with Nucleo simulation board)
- [ ] Navigation / position PID controller
- [ ] Obstacle avoidance logic
- [ ] Camera interface + AI inference (STM32Cube.AI)
- [ ] Power management (sleep/wake state machine)
- [ ] Telemetry transmission
