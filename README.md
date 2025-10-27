# Real-Time Automotive Control System

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![C](https://img.shields.io/badge/language-C-brightgreen.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)

A comprehensive real-time automotive Electronic Control Unit (ECU) simulation system featuring multi-process architecture, inter-process communication, and automated safety systems.

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [System Architecture](#system-architecture)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Usage](#usage)
- [Components](#components)
- [Sensor Data](#sensor-data)
- [Controller Logic](#controller-logic)
- [Crash Safety System](#crash-safety-system)
- [Web Dashboard](#web-dashboard)
- [Screenshots](#screenshots)
- [Technical Details](#technical-details)
- [Future Enhancements](#future-enhancements)
- [Contributing](#contributing)
- [License](#license)

## 🎯 Overview

This project simulates a real-time automotive ECU system with multiple sensors, automated controllers, and safety mechanisms. It demonstrates advanced systems programming concepts including:

- **Inter-Process Communication (IPC)** using shared memory
- **Multi-threading** with pthread
- **Signal handling** for process control
- **Real-time monitoring** with NCurses UI
- **Web-based dashboard** with HTTP server
- **Crash detection & emergency response**

## ✨ Features

- **Real-Time Sensor Simulation**
  - Engine temperature, speed, fuel level
  - Gear position, obstacle detection
  - Crash detection with 2% probability at high speeds

- **Automated Controllers**
  - Fan control based on engine temperature
  - AC control based on cabin temperature
  - Brake control for obstacle avoidance
  - Light & camera control for reverse gear
  - Fuel level monitoring
  - Safety system for crash response

- **Dual Interface**
  - NCurses-based terminal dashboard
  - Web-based remote monitoring (Port 8080)

- **Emergency Systems**
  - Automatic airbag deployment
  - Engine shutdown
  - Hazard lights activation
  - Horn/alarm alerts
  - Door auto-unlock
  - Server notification

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Shared Memory (IPC)                      │
│                    Key: 9876 (ECU Data)                     │
└─────────────────────────────────────────────────────────────┘
            ▲              ▲              ▲              ▲
            │              │              │              │
┌───────────┴───┐  ┌──────┴──────┐  ┌────┴─────┐  ┌────┴─────┐
│   sensor.c    │  │subsystem.c  │  │  UI.c    │  │server.c  │
│  (Main ECU)   │  │(Controllers)│  │(Terminal)│  │   (Web)  │
│               │  │             │  │          │  │          │
│ - Engine      │  │ - Fan       │  │ - NCurses│  │ - HTTP   │
│   Thread      │  │ - AC        │  │   Display│  │   Server │
│ - Signal      │  │ - Brake     │  │ - Real-  │  │ - Port   │
│   Handler     │  │ - Light     │  │   time   │  │   8080   │
│ - Crash       │  │ - Safety    │  │   Update │  │          │
│   Monitor     │  │ - Fuel      │  │          │  │          │
└───────────────┘  └─────────────┘  └──────────┘  └──────────┘
            ▲
            │
┌───────────┴───┐
│   signal.c    │
│ (Control Panel)│
│ - SIGUSR1/2   │
└───────────────┘
```

## 📦 Prerequisites

- **Operating System**: Linux (Ubuntu/Debian recommended)
- **Compiler**: GCC with pthread support
- **Libraries**:
  - `pthread` - POSIX threads
  - `ncursesw` - Wide character ncurses
  - Standard C libraries

### Install Dependencies (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install build-essential
sudo apt-get install libncursesw5-dev
```

## 🚀 Installation

1. **Clone the repository**
   ```bash
   git clone https://github.com/harshitkhandelwal17/Real_Time_Automotive_Control_System.git
   cd Real_Time_Automotive_Control_System
   ```

2. **Compile all components**
   ```bash
   gcc sensor.c -o sensor -pthread
   gcc subsystem.c -o subsystem -pthread
   gcc UI.c -o UI -lncursesw -pthread
   gcc server.c -o server -pthread
   gcc signal.c -o signal
   ```

3. **Make executable (optional)**
   ```bash
   chmod +x sensor subsystem UI server signal
   ```

## 🎮 Usage

### Step 1: Start the Main ECU Process
```bash
./sensor
```
Note the displayed **Process ID (PID)** - you'll need this for step 5.

### Step 2: Start the Controller Subsystem (New Terminal)
```bash
./subsystem
```

### Step 3: Start the Terminal UI (New Terminal)
```bash
./UI
```

### Step 4: Start the Web Server (New Terminal)
```bash
./server
```
Access the web dashboard at: `http://localhost:8080`

### Step 5: Control Ignition (New Terminal)
```bash
./signal
```
- Enter the PID from Step 1
- Press `1` to turn ignition ON
- Press `0` to turn ignition OFF

### Emergency Stop
- In the terminal UI, press `E` for emergency stop
- Or wait for automatic crash detection

## 📁 Components

### 1. sensor.c - Main ECU Process
- Creates shared memory for inter-process communication
- Spawns engine thread for sensor data generation
- Handles SIGUSR1 (ignition ON) and SIGUSR2 (ignition OFF)
- Monitors crash conditions
- Manages emergency shutdown

### 2. subsystem.c - Controller Process
Six independent controller threads:
- **Fan Controller**: Activates cooling when temp > 80°C
- **AC Controller**: Manages cabin temperature (>24°C)
- **Brake Controller**: Applies brakes for obstacles or high speed
- **Light Controller**: Controls reverse lights and camera
- **Safety Controller**: Handles crash response
- **Fuel Controller**: Monitors fuel levels

### 3. UI.c - Terminal Dashboard
- NCurses-based real-time interface
- Color-coded status indicators
- Seatbelt check before ignition
- Live sensor data display
- Manual emergency stop button
- Crash alert screen

### 4. server.c - Web Dashboard
- Multi-threaded HTTP server
- Port 8080
- Auto-refresh every 2 seconds
- Responsive HTML/CSS interface
- Real-time crash notifications

### 5. signal.c - Control Panel
- Sends SIGUSR1/SIGUSR2 to sensor process
- External ignition control

## 📊 Sensor Data

| Sensor | Range | Update Frequency |
|--------|-------|------------------|
| Engine Temperature | 80-100°C | Every 3 seconds |
| Engine Speed | 50-130 RPM | Every 3 seconds |
| Fuel Level | 1-100% | Every 3 seconds |
| Gear Position | 1-6 | Every 3 seconds |
| Inside Temperature | 20-50°C | Every 3 seconds |
| Obstacle Detection | 0/1 (25% chance) | Every 3 seconds |
| Crash Status | 2% at speed > 90 RPM | Real-time |

## ⚙️ Controller Logic

### Fan Controller
```c
if (engine_temp > 80.0)
    fan_status = ON;
else
    fan_status = OFF;
```

### AC Controller
```c
if (inside_temp > 24.0)
    ac_control = ON;
else
    ac_control = OFF;
```

### Brake Controller
```c
if (obstacle_detected || engine_speed > 100)
    brake_status = APPLIED;
```

### Light Controller
```c
if (gear_pos == 6) {  // Reverse gear
    back_light = ON;
    reverse_camera = ON;
}
```

### Fuel Controller
```c
if (fuel_level >= 75.0)
    fuel_status = FULL;
else if (fuel_level > 25.0)
    fuel_status = NORMAL;
else
    fuel_status = LOW;
```

## 🚨 Crash Safety System

When a crash is detected:

1. **Immediate Actions**:
   - Airbag deployment
   - Engine shutdown (ignition OFF)
   - Emergency stop activation

2. **Alert Systems**:
   - Hazard lights blinking
   - Horn/alarm activation
   - Emergency notification to server

3. **Safety Measures**:
   - Automatic door unlock
   - Crash screen display on UI
   - Web dashboard alert

## 🌐 Web Dashboard

Access the dashboard at `http://localhost:8080`

**Features**:
- Real-time sensor data display
- Color-coded status badges
- Engine ON/OFF status
- Crash alert banner
- Auto-refresh every 2 seconds
- Responsive design (mobile-friendly)

**Dashboard Sections**:
- Engine Metrics (Temperature, Speed, Fan)
- Fuel System (Level, Status)
- Braking (Brake Status, Obstacle Detection)
- Transmission (Gear, Lights, Camera)
- Safety Systems (Crash, Airbag, E-Stop)
- Emergency Response (Crash only)

## 🖼️ Screenshots

### Terminal Dashboard
![Terminal UI](https://via.placeholder.com/800x400?text=Terminal+Dashboard)

### Web Dashboard
![Web Dashboard](https://via.placeholder.com/800x400?text=Web+Dashboard)

### Crash Alert
![Crash Alert](https://via.placeholder.com/800x400?text=Crash+Alert)

## 🔧 Technical Details

### Shared Memory Structure
```c
typedef struct {
    ecu_sensor sensor;     // Sensor data
    ecu_control control;   // Control signals
    pthread_mutex_t lock;  // Synchronization
} ECU;
```

### IPC Key
- **Key**: 9876
- **Type**: System V Shared Memory
- **Size**: sizeof(ECU)

### Signal Handling
- **SIGUSR1**: Ignition ON
- **SIGUSR2**: Ignition OFF

### Threading
- **sensor.c**: 1 engine thread
- **subsystem.c**: 6 controller threads
- **server.c**: 1 thread per client connection

### Synchronization
- `pthread_mutex_lock()` for critical sections
- Prevents race conditions
- Ensures data consistency

## 🔮 Future Enhancements

- [ ] CAN Bus protocol implementation
- [ ] GPS integration for location tracking
- [ ] Cloud connectivity for remote diagnostics
- [ ] Machine learning for predictive maintenance
- [ ] Mobile app (iOS/Android)
- [ ] Database logging (SQLite/MySQL)
- [ ] OBD-II interface for real vehicles
- [ ] Voice alert system
- [ ] Advanced crash analytics
- [ ] Multi-vehicle network simulation

## 🤝 Contributing

Contributions are welcome! Please follow these steps:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/YourFeature`)
3. Commit your changes (`git commit -m 'Add YourFeature'`)
4. Push to the branch (`git push origin feature/YourFeature`)
5. Open a Pull Request

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 👥 Authors

- **Harshit Khandelwal** - [GitHub](https://github.com/harshitkhandelwal17)

## 🙏 Acknowledgments

- POSIX threads documentation
- NCurses library documentation
- Automotive ECU system references
- Open source community

## 📧 Contact

For questions or support, please open an issue on GitHub or contact the maintainer.

---

**⭐ If you find this project useful, please consider giving it a star!**

## 🛠️ Troubleshooting

### Issue: "shmget failed"
**Solution**: Make sure `sensor.c` is running first as it creates the shared memory.

### Issue: "bind failed" on server.c
**Solution**: Either run with `sudo` or change port from 80 to 8080 (already done in updated code).

### Issue: NCurses display issues
**Solution**: Install `libncursesw5-dev` and recompile with `-lncursesw` flag.

### Issue: Thread creation failed
**Solution**: Ensure you're compiling with `-pthread` flag.

### Issue: Crash not detected
**Solution**: Crash has 2% probability when speed > 90 RPM. Keep engine running and wait.

---

**Made with ❤️ for Automotive Systems Learning**
