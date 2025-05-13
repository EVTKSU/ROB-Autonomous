# EVT‑Teensy Firmware

**Table of Contents**

1. [PlatformIO Setup (IMPORTANT)](#platformio-setup-important)  
2. [Project Structure](#project-structure)  
3. [Build & Flash Workflow](#build--flash-workflow)  
4. [Module Overview](#module-overview)  
   * [State Machine](#state-machine-evt_statemachine)  
   * [Ethernet / Telemetry](#ethernet--telemetry-evt_ethernet)  
   * [RC Interface](#rc-interface-evt_rc)  
   * [VESC Driver](#vesc-driver-evt_vescdriver)  
   * [Odrive Driver](#odrive-driver-evt_odriver)  
   * [Autonomous Mode](#autonomous-mode-evt_automode)  
5. [Runtime Flow](#runtime-flow)  
6. [Extending the Code Base](#extending-the-code-base)  
7. [Troubleshooting / FAQ](#troubleshooting--faq)  

---

## PlatformIO Setup (IMPORTANT)

The **TeensyDevelopment** folder is a stand‑alone PlatformIO project.

* **Only one file may live in `src/` at a time** – that file must be `main.cpp`.  
* All reusable code goes in `lib/` as named library folders (e.g. `lib/EVT_RC/…`).  
* **Running / flashing code**

  1. Open the **TeensyDevelopment** folder in **VS Code** *by itself.*  
  2. Wait for PlatformIO to finish indexing.  
  3. Edit `src/main.cpp`.  
  4. Click the ✔️ (Build) to compile, ➡️ (Upload) to flash, 🔌 (Serial) for monitor.

* **Storing prototypes / experiments**

  * Drop extra sketches in `TeensyTestCode/`.  
  * Comment your name at the top, then copy‑paste the file into `src/main.cpp` when you actually want to run it.

---

## Project Structure

PlatformIO/

└── TeensyDevelopment/

├── platformio.ini ← board & build settings

├── src/

│   └── main.cpp ← entry point (state‑machine loop)

└── lib/

├── EVT_StateMachine/ ← central state engine

├── EVT_Ethernet/ ← UDP telemetry helpers

├── EVT_RC/ ← SBUS RC interface

├── EVT_VescDriver/ ← dual‑VESC UART driver

├── EVT_ODriver/ ← ODrive UART driver

└── EVT_AutoMode/ ← autonomous helpers


---

## Build & Flash Workflow

1. **Connect** the Teensy and open VS Code.  
2. **Configure** any IP/MAC changes in `EVT_Ethernet.cpp` (default `192.168.0.177`).  
3. **Build** (✔️) – PlatformIO compiles every library under `lib/`.  
4. **Upload** (➡️) – Flashes the Teensy; the board will reboot.  
5. **Monitor** (🔌) – Opens serial @ 9600 baud; watch debug prints.  

> **Note:** Ethernet MAX PACKET SIZE in the Teensy core must be raised to 64 bytes (default is 36).  
> Edit `<Arduino‑core>/libraries/NativeEthernet/src/utility/util.h` if you have compile‑time truncation issues.

---

## Module Overview

### State Machine (`EVT_StateMachine`)

| Enum State | Purpose |
|------------|---------|
| `NONE`     | Pre‑boot / undefined |
| `INIT`     | Hardware bring‑up (Ethernet, drivers) |
| `CALIB`    | ODrive calibration sequence |
| `RC`       | Manual remote‑control mode |
| `AUTO`     | Autonomous mode running UDP commands |
| `ERR`      | Fatal error – motors stopped, requires user reset |

Modules call `SetState()` or `SetErrorState()` to transition. `StateToString()` converts the enum to a printable string.

---

### Ethernet  /  Telemetry (`EVT_Ethernet`)

* Initializes **NativeEthernet** and a global `EthernetUDP Udp` object.  
* `sendTelemetry()` — formats six floats and broadcasts to `192.168.0.132:8888`.  
* `receiveUdp()` — non‑blocking; returns a `std::string` packet or empty.

---

### RC Interface (`EVT_RC`)

* Uses **SBUS** on `Serial2` @ 100 kBd.  
* Exposes `uint16_t channels[10]` array.  
* `updateSbusData()` refreshes the channel buffer – called every loop.

---

### VESC Driver (`EVT_VescDriver`)

* Two **VescUart** objects (`Serial1`, `Serial5`).  
* Maps `channels[1]` (throttle) to ±7500 RPM with neutral dead‑band.  
* Updates global `vescDebug` string with live RPM & voltage.

---

### Odrive Driver (`EVT_ODriver`)

* UART on **Serial6**.  
* Handles motor & encoder offset calibration (triggered via `channels[5]`).  
* Supports error clearing / re‑cal via `channels[4]`.  
* Controls steering position via `channels[3]`.  
* Publishes `odrvDebug` for telemetry prints.

---

### Autonomous Mode (`EVT_AutoMode`)

* Polls UDP for commands: `steering,throttle,emergency`.  
* On entry, captures current ODrive pos as center.  
* Maps throttle% to RPM and holds steering center while emergency flag is 0.  
* If `emergency == true` ➜ calls `SetErrorState()`.

---

## Runtime Flow

1. **setup()**  
   * `SetState(INIT)` ⟶ Ethernet / SBUS / drivers init.  
   * `SetState(RC)` – ready for manual driving.

2. **loop()**  
   * Always refresh SBUS.  
   * `switch(GetState())`  
     * **RC** – if `channels[6] > 1000` ➜ `AUTO`, else run VESC+ODrive updates.  
     * **AUTO** – if `channels[6] < 1000` ➜ back to `RC`; otherwise run UDP autonomous routine.  
     * **ERR** – wait for operator reset (`channels[4]` high w/ auto switch low).  

3. **Telemetry** – every autonomous loop sends telemetry; RC loops can add later.

---

## Extending the Code Base

* **New module?** Create `lib/EVT_MyModule/` with `EVT_MyModule.h/.cpp`.  
* **Error handling** – call `SetErrorState("Module","Reason")`.  
* **Documentation** – each library needs a `README.md` explaining its API.  
* **Branches** – develop on a new Git branch; open PRs for review.

---

## Troubleshooting / FAQ

| Problem | Fix |
|---------|-----|
| **“multiple definition of operator new”** | Add `-Wl,--allow-multiple-definition` to `build_flags` in `platformio.ini`. |
| **Ethernet packet cut at 36 B** | Increase `UDP_TX_PACKET_MAX_SIZE` to 64 in Teensy **NativeEthernet** core. |
| **State machine keeps breaking** | Follow enum + `switch` template in `main.cpp`; keep module code non‑blocking. |
| **No SBUS data** | Confirm `Serial2` wiring and 100 kBd 8E2 settings. |
| **ODrive never reaches CLOSED_LOOP** | Check power, hall/encoder cables, and run calibration trigger (`channels[5]`). |

---

> *“Making a proper state machine setup nukes the code every time – until this one.”*  

