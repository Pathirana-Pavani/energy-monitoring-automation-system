# Energy Monitoring & Automation System

A smart IoT-based energy management system designed to monitor real-time electrical parameters and provide remote control capabilities. This project utilizes a **dual-microcontroller architecture**, where an **Arduino** handles sensor data acquisition and load switching, while an **ESP32** acts as a WiFi gateway to sync data with **Google Firebase**.

## 📌 Overview

This system provides visibility into energy consumption and enforces efficiency through automation.

* **Sensing:** Measures instantaneous power and calculates accumulated energy (Daily/Total).
* **Connectivity:** Syncs data to the cloud (Firebase Realtime Database) for remote monitoring.
* **Control:** Allows users to toggle appliances remotely via the cloud or automatically based on energy thresholds.
* **Safety:** Includes zero-crossing detection for precise measurement.

## 🚀 Key Features

* **Real-Time Monitoring:** Tracks Power (W), Daily Energy (kWh), and Total Energy (kWh).
* **Dual-Core Architecture:**
    * **Arduino:** Dedicated to timing-critical sensor readings and relay control.
    * **ESP32:** Dedicated to WiFi connectivity and database management.
* **Remote Control:** Toggle up to 3 separate loads remotely via Firebase.
* **Automated Load Shedding:** Automatically turns off appliances if daily energy consumption exceeds defined thresholds:
    * *Level 1 Warning:* 0.1 kWh
    * *Level 2 Cutoff:* 0.3 kWh
* **Cloud Integration:** Real-time bi-directional communication using Google Firebase.

## 🛠️ Tech Stack

### Hardware
* **Sensing & Control:** Arduino Uno (or compatible AVR board).
* **IoT Gateway:** ESP32 Development Board.
* **Sensors:** Voltage Sensor (ZMPT101B/Divider), Current Sensor (ACS712).
* **Actuators:** 3-Channel Relay Module.

### Software
* **Firmware:** C++ (Arduino IDE).
* **Cloud Backend:** Google Firebase Realtime Database.
* **Libraries:** `Firebase_ESP_Client`, `WiFi.h`, `Wire.h`.

## 🔌 Circuit & Pinout

The system uses **UART Serial** to communicate between the Arduino and ESP32.

### 1. Arduino Pinout (Sensing & Control)
| Component | Pin | Description |
| :--- | :--- | :--- |
| **Voltage Sensor** | A0 | Analog Voltage Input |
| **Current Sensor** | A1 | Analog Current Input |
| **Zero Cross (V)** | D2 | Voltage Zero Crossing Interrupt |
| **Zero Cross (I)** | D3 | Current Zero Crossing Interrupt |
| **Load 1** | D6 | Relay Control Channel 1 |
| **Load 2** | D4 | Relay Control Channel 2 |
| **Load 3** | D5 | Relay Control Channel 3 |
| **Serial Tx** | D1 (Tx) | Sends data to ESP32 (Connect to ESP32 RX2) |
| **Serial Rx** | D0 (Rx) | Receives commands (Connect to ESP32 TX2) |

### 2. ESP32 Pinout (Gateway)
| Component | Pin | Description |
| :--- | :--- | :--- |
| **UART RX** | GPIO 16 (RX2) | Receives Data from Arduino |
| **UART TX** | GPIO 17 (TX2) | Sends Commands to Arduino |

## ⚙️ Setup & Installation

### 1. Firebase Configuration
1. Create a project on the [Firebase Console](https://console.firebase.google.com/).
2. Create a **Realtime Database**.
3. Set database rules to `true` for testing (or configure auth).
4. Obtain your **Project API Key** and **Database URL**.

### 2. Firmware Configuration

**For ESP32 (`esp32_control.cpp`):**

Update the credentials at the top of the file:
```cpp
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASS"
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "https://your-project.firebaseio.com/"
```

**For Arduino (`Arduino_Control.ino`):**

Ensure the `COMPONENT_PINS` match your physical relay connections.
```cpp
const int COMPONENT_PINS[3] = {6, 4, 5};
```

### 3. Database Structure

The system automatically reads/writes to these paths:

* `monitoring/data`: Stores live sensor values (Power, Energy, States).
* `monitoring/command`: Listens for remote toggle commands (e.g., `TOGGLE:1:ON`).

## 📊 Usage

1. **Power Up:** Connect the system to a power source.
2. **Sync:** The ESP32 will connect to WiFi and Firebase (Output visible on Serial Monitor).
3. **Monitor:**
   * The Arduino calculates power usage and sends it to the ESP32 via Serial.
   * The ESP32 pushes this data to Firebase under `monitoring/data`.
4. **Control:**
   * Change the value in Firebase `monitoring/command` to toggle loads.
   * If usage exceeds `0.3 kWh`, the Arduino automatically sheds loads.

## 👥 Contributors

This project was developed by:

| Name | Email |
| --- | --- |
| **Dodandeniya D. G. J. K.** | [e20085@eng.pdn.ac.lk](mailto:e20085@eng.pdn.ac.lk) |
| **Edirisinghe H. K. D.** | [e20091@eng.pdn.ac.lk](mailto:e20091@eng.pdn.ac.lk) |
| **Pathirana B. P. P.** | [e20281@eng.pdn.ac.lk](mailto:e20281@eng.pdn.ac.lk) |

---

*University of Peradeniya, Faculty of Engineering*
