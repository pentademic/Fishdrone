# Contributing to FishDrone

Thank you for your interest in contributing! This document explains how to work with the codebase.

---

## Getting Started

1. **Fork** this repository and create a feature branch from `main`:
   ```bash
   git checkout -b feature/my-feature
   ```
2. Make your changes following the guidelines below.
3. Open a **Pull Request** with a clear description of what you changed and why.

---

## Branch Naming

| Type | Pattern | Example |
|---|---|---|
| New feature | `feature/<short-description>` | `feature/gps-driver` |
| Bug fix | `fix/<short-description>` | `fix/tof-timeout` |
| Documentation | `docs/<short-description>` | `docs/wiring-update` |
| Refactor | `refactor/<short-description>` | `refactor/nav-pid` |

---

## Code Style

### C / C++ (STM32CubeIDE)

- Follow the ST-generated code style (snake_case for variables, ALL_CAPS for macros).
- Add a Doxygen file header to every new `.c` / `.h` file:
  ```c
  /**
   * @file my_module.c
   * @brief One-line description.
   */
  ```
- Keep functions short and single-purpose.
- Use `HAL_` APIs wherever possible; avoid direct register access unless performance demands it.

### Arduino (`.ino`)

- Use `camelCase` for variables and functions.
- Add a comment block at the top of every sketch explaining purpose, hardware, and wiring.
- Avoid `delay()` in production code; prefer non-blocking patterns with `millis()`.

---

## Commit Messages

Use the [Conventional Commits](https://www.conventionalcommits.org/) format:

```
<type>(<scope>): <short summary>

[optional body]
```

Examples:
```
feat(tof): add multi-sensor address assignment
fix(gps): handle partial NMEA sentences
docs(wiring): add motor driver wiring table
```

---

## Testing

Before submitting a PR:

- **Hardware test:** Verify the change on a real Nucleo / STM32H7 board if possible.
- **Static analysis:** Run the STM32CubeIDE built-in analyser or `cppcheck` locally.
- **Serial output:** Confirm Serial Monitor output matches expected behaviour.

---

## Repository Structure

See the top-level [README.md](README.md) for the full directory layout.

New firmware modules should be added under `firmware/<module-name>/`.
New documentation pages should be added under `docs/`.

---

## Questions

Open a GitHub Issue or contact the team leads listed in [README.md](README.md).
