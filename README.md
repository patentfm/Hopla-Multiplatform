# Interactive Balance & Core Trainer (BLE + Mobile App)

This repository contains a **monorepo** for an interactive fitness toy/device designed to strengthen **core muscles, balance, and postural stability**, with a particular focus on counteracting the negative effects of a sedentary lifestyle.

The product consists of a **compact, spring-based balance mat** (available in child and adult sizes) with a rigid load-distribution board on top. Inside the board, a **Bluetooth Low Energy (BLE) module with an accelerometer** measures motion, tilt, balance shifts, and dynamic load changes while the user jumps, rocks, or stabilizes their body.

Sensor data is transmitted in real time to a **cross-platform mobile application** (Android / iOS), which turns physical exercise into **interactive games and mini-challenges**. The user trains balance and deep stabilizing muscles by reacting to in-app tasks while standing, jumping, or leaning on the mat.

---

## Key Goals

- Improve **core strength, balance, and coordination**
- Encourage **movement through play** for children and adults
- Combine **embedded systems + mobile gamification**
- Enable **data-driven training**, progress tracking, and feedback

---

## Monorepo Structure

This repository contains all major components of the system:

- **Embedded firmware**
  - BLE device firmware (accelerometer, motion processing)
  - Power management and low-energy operation
  - OTA / DFU update support
- **Mobile application**
  - Cross-platform app (Android / iOS)
  - BLE communication and device management
  - Game logic, UI/UX, and user feedback
- **Shared resources**
  - Communication protocols (BLE GATT)
  - Documentation and diagrams
  - Development tools and scripts

---

## Technologies (Planned)

- Bluetooth Low Energy (BLE)
- Accelerometer-based motion analysis
- Embedded ARM firmware
- Cross-platform mobile development (e.g. Flutter)
- Real-time sensor streaming and game logic

---

## Target Use Cases

- Home fitness and rehabilitation
- Children’s movement and balance training
- Gamified physical activity for sedentary users
- Educational and playful exercise experiences

---

## Project Vision

The goal is to create a **safe, scalable, and commercially viable** interactive fitness platform that merges **physical hardware with digital gameplay**, encouraging healthier movement habits through engaging experiences.

