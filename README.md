# Hestia — Smart Home System

Smart house project for the **Embedded Software for the Internet of Things** course @ University of Trento.

Hestia turns a Raspberry Pi 5 into the controller of a small smart house. A single C++ application coordinates every subsystem, while a lightweight Python/Flask web app exposes a remote control panel. The house can be operated in three ways at the same time: through physical sensors, through an infrared remote and through the web interface.

**Main features**
- 🚨 Alarm system with PIR motion sensors, siren (buzzer + LED) and 5s exit delay
- 💡 Automated lighting that follows motion, with an optional manual mode
- 🚗 AI-driven license-plate recognition that opens the gate and disarms the alarm for authorized cars
- 🌡️ Automated climate control (air conditioning + heating) driven by a temperature/humidity sensor
- 📺 Infrared remote control over the whole house
- 🌐 Web application control over the whole house

The project can be built in two flavours:
- **`release`** — runs on the real Raspberry Pi 5 hardware (GPIO, camera, servo, sensors).
- **`sim`** — a full **simulation mode** that compiles and runs on any Linux/macOS machine without any hardware attached, faking sensor input so the whole logic can be tested from a normal PC.

---

## Presentation & Video

- 📊 **PowerPoint presentation:** (./HestiaV1-Presentation.pdf)
- ▶️ **YouTube video:** _[add link here]_

---

## Requirements

### Hardware Requirements

The reference setup runs on a **Raspberry Pi 5** wired according to the diagram in [`PinDiagram_RaspberryPi5.jpg`](./PinDiagram_RaspberryPi5.jpg).

| Component | Role | Default GPIO (BCM) |
|---|---|---|
| Raspberry Pi 5 | Main controller | — |
| PIR motion sensor (alarm) | Intrusion detection | 17 |
| PIR motion sensor (lights) | Presence detection for lighting | 26 |
| Passive buzzer | Alarm siren | 27 |
| LED — alarm | Alarm/siren indicator | 22 |
| LED — lights | Lighting | 16 |
| LED — air conditioning | AC actuator indicator | 23 |
| LED — heating | Heating actuator indicator | 5 |
| IR receiver | Remote-control input | 25 |
| Servo motor (SG90) | Gate open/close | 24 |
| DHT11 | Temperature + humidity sensor | (kernel IIO driver) |
| USB webcam | License-plate capture | `/dev/video0` |
| 2N2222 Transistor | AC fan transistor | 23 |
| Fan blade and 3-6V motor | AC fan | - |

> Pin assignments are defined in [`include/houseControl.h`](./include/houseControl.h) and can be changed there.

> **Note:** the DHT11 is read through the kernel **IIO** interface (`/sys/bus/iio/.../in_temp_input` and `in_humidityrelative_input`), so the corresponding device-tree overlay must be enabled on the Pi.

### Software Requirements

**To run in simulation mode (any Linux/macOS machine, no hardware):**
- `g++` with C++17 support
- `make`
- POSIX threads (`-lpthread`)

**To run on the Raspberry Pi 5 (`release` build):**
- `g++` with C++17 support and `make`
- [`libgpiod`](https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/) **v2** (`-lgpiod`) — GPIO / edge-event handling
- [`lgpio`](https://abyz.me.uk/lg/lgpio.html) (`-llgpio`) — PWM (buzzer) and servo control
- [OpenVINO](https://docs.openvino.ai/) — runs the YOLO plate-detection model
- [OpenCV 4](https://opencv.org/) (`opencv4` via `pkg-config`) — image processing
- [Tesseract OCR](https://github.com/tesseract-ocr/tesseract) + [Leptonica](http://www.leptonica.org/ - tesseract dependency) (`-ltesseract -lleptonica`) — reads the plate text
- The trained detection model, shipped in [`models/`](./models/) (`best.xml` + `best.bin`)

**For the web interface (both modes):**
- Python 3
- [Flask](https://flask.palletsprojects.com/) (`pip install flask`)

---

## Project Layout

```
Hestia/
├── README.md
├── Makefile                      # build rules: `sim` and `release`
├── PinDiagram_RaspberryPi5.jpg   # wiring diagram
│
├── include/                      # public headers / module interfaces
│   ├── houseControl.h            # GPIO pin map + high-level control API
│   ├── motion_sensor.h           # PIR sensor interface
│   ├── led.h                     # LED control interface
│   ├── buzzer.h                  # buzzer/siren interface
│   ├── ir_sensor.h               # NEC timing params + raw IR capture
│   ├── ir_decoder.h              # NEC frame decoding interface
│   ├── ir_remote.h               # remote key codes -> actions mapping
│   ├── gate.h                    # servo gate interface
│   ├── Dht11.h                   # temperature/humidity sensor interface
│   └── cameraYOLO.h              # camera + plate-recognition interface
│
├── src/                          # implementation
│   ├── main.cpp                  # entry point: global state, main loop, UDP web-command thread, temperature monitor
│   ├── houseControl.cpp          # orchestrator: alarm/siren/lights/gate/AC/heating logic + plate access control
│   ├── motion_sensor.cpp         # PIR motion sensors via libgpiod edge events
│   ├── led.cpp                   # LED actuators via libgpiod
│   ├── activeBuzzer.c            # active-buzzer driver (libgpiod, on/off)
│   ├── passiveBuzzer.cpp         # passive-buzzer siren (lgpio PWM sweep)
│   ├── ir_sensor.cpp             # interrupt-driven NEC raw-frame capture
│   ├── ir_decoder.cpp            # NEC protocol decoder (leader, 32 bits, checksum, debounce)
│   ├── ir_remote.cpp             # key->action table and dispatch
│   ├── gate.cpp                  # SG90 servo gate (gradual sweep) via lgpio
│   ├── Dht11.cpp                 # DHT11 reader via kernel IIO sysfs
│   ├── cameraYOLO.cpp            # OpenVINO YOLO detection + Tesseract OCR
│   └── server.py                 # Flask web server + UI (talks to C++ over UDP)
│
├── models/                       # trained plate-detection model
│   ├── best.xml                  # OpenVINO model topology
│   └── best.bin                  # OpenVINO model weights
│
└── build/                        # build output (created by make) -> build/hestiaV1
```

### Architecture overview

```
        IR Remote ──┐
                    ├──► main.cpp  (global atomic state + main loop)
   Web UI (Flask) ──┘            │
        server.py  ──UDP:12345──►│
                                 ├──► houseControl.cpp  (orchestrator)
                                 │         ├── alarm + siren
   Sensors / Actuators ◄─────────┤         ├── lights (auto / manual)
   (PIR, LED, buzzer,            │         ├── gate (servo)
    DHT11, servo, camera)        │         ├── AC / heating (DHT11)
                                 │         └── camera (YOLO + plate OCR)
```

The application keeps the whole house state in a set of thread-safe `std::atomic` flags defined in `main.cpp` (e.g. `alarmOn`, `sirenOn`, `lightsOn`, `gateOpen`, `acOn`, `heatingOn`). Three independent command sources — the **IR remote**, the **web UI** (via a UDP server on port `12345`) and the **sensor threads** — update this shared state, while the main loop and the listener threads react to it. The Flask server (`server.py`) only acts as a REST → UDP bridge to the C++ core.

---

## How to Build, Burn and Run

The whole C++ application is built with the provided [`Makefile`](./Makefile). The output binary is `build/hestiaV1`.

### 1. Build

**Simulation mode** (any Linux/macOS machine, no hardware required):

```sh
# From the root of the project
make sim
```

This defines the `-DSIM` macro, excludes all the hardware libraries and links only against pthreads. Sensors, IR keys and the camera are faked in software, so you can test the whole control logic on a normal PC.

**Release mode** (on the Raspberry Pi 5, with all hardware connected):

```sh
make release
```

This links against `libgpiod`, `lgpio`, OpenVINO, OpenCV and Tesseract and drives the real GPIO pins, servo and camera.

To clean the build artifacts:

```sh
make clean
```

> The OpenVINO path in the `Makefile` (`OPENVINO_DIR`) may need to be adjusted to match the install location on your Raspberry Pi.

### 2. "Burn" / Deploy

Unlike a microcontroller, the Raspberry Pi 5 runs a full Linux OS, so there is **no firmware to flash**. Deployment simply means copying the project onto the Pi and building it there:

```sh
# On your PC: copy the project to the Raspberry Pi
scp -r Hestia/ pi@<raspberry-ip>:~/

# On the Raspberry Pi
cd ~/Hestia
make release
```

Make sure the wiring matches [`PinDiagram_RaspberryPi5.jpg`](./PinDiagram_RaspberryPi5.jpg) and that the DHT11 IIO overlay is enabled before running.

### 3. Run

Start the C++ core (it also opens the UDP command server on port `12345`):

```sh
# Simulation
./build/hestiaV1

# Release
cd build
./hestiaV1
```
> The camera loop loads the model from `../models/best.xml` (relative path), so on the real hardware run the binary from the `build/` directory (or adjust the path accordingly).

In a second terminal, start the web interface:

```sh
cd src
python3 server.py
```

Then open a browser at **`http://localhost:5678`** to reach the Hestia control panel.

---

## User Guide

Once `hestiaV1` is running you can control the house in three ways.

### A) Web interface

Open `http://localhost:5678`. The dashboard shows live status (it polls the system every 1.5s) and lets you:

| Card | Control | Effect |
|---|---|---|
| **Alarm system** | `ARM / DISARM ALARM` | Arms or disarms the alarm. Arming starts a 5s exit delay, then the camera and motion listener. |
| **Main Gate** | `OPEN / CLOSE GATE` | Opens or closes the servo-driven gate. |
| **Lighting** | `Automatic mode` toggle | Switches between motion-driven (auto) and manual lighting. |
| **Lighting** | `TURN ON / OFF LIGHTS` | Turns the lights on/off (only available in manual mode). |
| **Air Conditioning** | `Automatic mode` toggle | Switches AC between automatic (temperature-driven) and manual. |
| **Air Conditioning** | `AC ON / OFF` | Turns the AC on/off (only in manual mode). |
| **Heating** | `Automatic mode` toggle | Switches heating between automatic and manual. |
| **Heating** | `HEATING ON / OFF` | Turns the heating on/off (only in manual mode). |
| **System** | ⏻ `Shutdown` | Cleanly shuts down the whole system. |

### B) Infrared remote (Elegoo NEC remote)

| Button | Action |
|---|---|
| **POWER** | Shut down the whole system |
| **PLAY/PAUSE** | Arm / disarm the alarm |
| **FUNC/STOP** | Toggle lighting mode (manual ↔ motion) |
| **0** | Toggle heating mode (manual ↔ automatic) |
| **1** | Toggle lights on/off |
| **2** | Open / close the gate |
| **3** | Toggle AC mode (manual ↔ automatic) |
| **4** | Toggle AC on/off |
| **5** | Toggle heating on/off |

### C) Automatic behaviour

- **Alarm:** when armed, any motion detected by the alarm PIR triggers the siren (LED blink + buzzer sweep).
- **Lights (auto mode):** motion turns the lights on; after motion ends they switch off following a short delay.
- **Climate (auto mode):** the DHT11 is polled periodically — the **AC turns on above 31 °C**, the **heating turns on below 30 °C**.
- **License-plate recognition:** while the alarm is armed, the webcam continuously looks for license plates. When an **authorized plate** (configured in `houseControl.cpp`, default `CZ889KF`) is recognized, the system disarms the alarm and automatically opens the gate for ~10s before closing it again.

### Shutting down

Press the remote **POWER** button or use the web **Shutdown** button to make the system stop all listener threads, turn off every actuator and release the GPIO resources before exiting.

---

## Team Members

Project developed by:

- **Leonardo Tartini - 245119**
- **Giovanni Todesco - 244645**
- **Mikele Golemi - 243170**
- **Francesco Zadra - 244544**
