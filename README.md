# F.I.S.H D.R.O.N.E

**Fishing Illegal Surveillance and Hunting Drone for Real-time Oceanic Navigation and Enforcement**

An autonomous marine robot designed to monitor protected maritime zones and detect illegal fishing activity, built around the STM32H7 microcontroller.

---

## Overview

FishDrone is a student engineering project developed for the I-Novgames 2024 hackathon organised by STMicroelectronics (theme: *Biodiversity*). The robot is an autonomous surface vessel that patrols a configurable sea area, detects nearby vessels via radar and camera-based AI, and reports potential illegal fishing activity in real time.

Key capabilities:

| Function | Technology |
|---|---|
| Long-range vessel detection | Radar (simulated via Nucleo board) |
| Obstacle avoidance | 5× VL53L1X Time-of-Flight sensors |
| Position control | GPS + dual propeller motors |
| Vessel identification | Camera + AI (STM32H7) |
| Pan/tilt camera turret | 2× MG996R servo motors |
| Low-power standby mode | Radar + TOF only active |
| Energy autonomy | Solar panel + battery |

---

## Repository Structure

```
Fishdrone/
├── README.md               ← This file
├── CONTRIBUTING.md         ← Contribution guidelines
├── .gitignore              ← Build artifact exclusions
│
├── docs/                   ← Technical documentation (Markdown)
│   ├── specs.md            ← Hardware specs and component list
│   ├── wiring.md           ← Wiring guide (TOF sensor, GPS, …)
│   └── architecture.md     ← System architecture overview
│
├── firmware/               ← Embedded firmware source code
│   ├── README.md           ← Build & flash instructions
│   └── tof/
│       └── tof_sensor.ino  ← VL53L1X TOF sensor driver (Arduino/STM32duino)
│
└── (project documents)     ← Original .docx/.xlsx planning files
```

---

## Hardware

- **Microcontroller:** STM32H7 (high-performance, runs AI inference)
- **TOF sensors:** VL53L1X × 5 (obstacle avoidance, up to 8–10 m)
- **Camera:** MB1683 or MB1379 (5 Mpx, visible/IR)
- **GPS module:** TESEO-LIV3F
- **Radar:** Band-S / Band-X (simulated by STM32 Nucleo board in prototype)
- **Servo motors:** MG996R × 2 (camera turret pan/tilt)
- **Propulsion:** 2× DC underwater thruster motors
- **Power:** Solar panel + LiPo battery

---

## Getting Started

### Prerequisites

- [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) **or** [Arduino IDE](https://www.arduino.cc/en/software) with the STM32duino board package
- USB cable for flashing
- Required Arduino libraries: `VL53L1X` (install via Library Manager), `Wire`

### Build & Flash

See [`firmware/README.md`](firmware/README.md) for detailed instructions.

---

## Documentation

| Document | Description |
|---|---|
| [`docs/specs.md`](docs/specs.md) | Full hardware specification and BOM |
| [`docs/wiring.md`](docs/wiring.md) | Pin-by-pin wiring diagrams |
| [`docs/architecture.md`](docs/architecture.md) | Software/hardware architecture |

Original project documents (French) are also available in the root:
- `Cahier des charges - formalisé.docx` — Formal specification
- `Report Fishdrone.docx` — Full project report
- `TOF.docx` — TOF sensor integration notes

---

## Team

| Name | Role |
|---|---|
| Manaiki Laut | Project Manager |
| Arthur Boudehent | Technical Lead |
| Julien Zhang | Deliverables Manager |
| Alexandre Paul | Communication |
| Nicolas Gallard | Mechanical / 3D Design |
| Adam Berrada | Development |

**Tutor:** Thierry Gaidon  
**Institution:** I-Novgames 2024 — STMicroelectronics Hackathon

---

## License

This project was developed as part of an academic competition. No open-source license has been assigned yet. Contact the team before reusing any part of this work.