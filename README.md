# 🌡️ Temperature Control System with User Interface (STM32 + FreeRTOS + Python)

## 📖 Project Overview
This project implements a **closed-loop temperature control system (SISO)** based on the **STM32** platform. The system integrates a hardware layer (microcontroller, sensors, actuators) with advanced embedded software running under the **FreeRTOS** real-time operating system. The solution is complemented by a dedicated PC desktop application (HMI) that allows for remote process monitoring and PID controller tuning.

The main goal of the project is to maintain a set temperature of a heating element despite external disturbances, utilizing a **PID algorithm** and an auxiliary cooling fan.

---

## 🏗️ System Architecture

### 1. Hardware Layer
The core of the system is the **STM32L476RG** (ARM Cortex-M4) Nucleo development board. The peripherals are divided into logical functional blocks:

* **Actuators:**
    * **Heater:** High-power resistor controlled via a **PWM** (Pulse Width Modulation) signal through a MOSFET, allowing for precise power regulation.
    * **Fan:** Controlled via a digital signal (On/Off) through a MOSFET. Acts as a cooling element and a generator of deterministic disturbances.
* **Sensors:**
    * **BMP280:** Digital temperature sensor communicating via the **I2C** bus. Provides precise measurement of the Process Value (PV).
* **Local HMI:**
    * **LCD Display (2x16):** Connected via a PCF8574 expander (I2C), displaying the current temperature and the setpoint.
    * **Rotary Encoder:** Uses the STM32 hardware timer encoder mode for precise temperature setpoint adjustment.
* **Signaling:**
    * **LEDs:** Status indicators for **READY** (target reached within ±5%), **HEATING** (PWM active), and **COOLING** (Fan active).

### 2. Embedded Firmware
The firmware is written in C/C++ and is based on **FreeRTOS**, ensuring time determinism for critical control tasks.

#### Task Structure:
* **`PID_Task` (High Priority):**
    * Executes the main control loop at **100 Hz**.
    * Responsible for PID output calculation, **Anti-Windup**, fan hysteresis logic, and a software **Interlock** preventing simultaneous heating and cooling.
* **`Communication_Task` (Normal Priority):**
    * Manages data exchange via UART.
    * **Command Reception:** Uses **Interrupt-based** handling for parsing commands (e.g., changing $K_p, K_i, K_d$ gains).
    * **Telemetry Transmission:** Cyclically sends system state in **JSON** format.
* **`Interface_Task` (Low Priority):**
    * Handles the local user interface.
    * **Synchronization:** The BMP280 sensor reading is triggered by a hardware timer via a **Binary Semaphore**, offloading the CPU from constant I2C polling.

#### Data Safety:
Data exchange between threads is protected by **Mutexes** (Mutual Exclusion) to prevent race conditions and ensure data consistency.

### 3. PC Monitor Application (Software)
The desktop application is written in **Python** (using `Tkinter`, `Matplotlib`, `PySerial`) and serves as an advanced operator panel.

#### Key Features:
* **Visualization:** Real-time plotting of Setpoint vs. Measured Value and Control Error.
* **Online Tuning:** Dynamic transmission of new PID coefficients to the MCU without a reset.
* **System Status:** Virtual LED indicators (Fan, Ready) and statistics (PWM Duty Cycle, CRC Errors).
* **Communication Protocol:** Data is transmitted in JSON format secured by a **CRC-8 checksum**, ensuring transmission integrity.

---

## 📉 Control Theory Aspects

A complete identification and synthesis process was performed for the control system:

1.  **Object Identification:**
    * An open-loop active experiment (step response) was conducted.
    * The object was approximated as a **First-Order Plus Dead Time (FOPDT)** inertial model.
    * **Identified Parameters:**
        * Static Gain: $K \approx 0.88$
        * Time Constant: $T \approx 149s$
        * Delay: $\theta \approx 9.8s$
    * The model quality was confirmed with a high determination coefficient ($R^2 \approx 0.9997$).

2.  **Controller Tuning:**
    * Analytical methods (**Pole Placement / Lambda Tuning**) and numerical tuning (**PID Tuner**) were used to find a compromise between rise time and overshoot.

3.  **Verification:**
    * Final Settings: $K_p = 59.20$, $K_i = 0.34$.
    * These settings ensure stable operation and fast disturbance compensation.

---

## 🛠️ Getting Started

### Prerequisites
* **Hardware:** STM32 Nucleo-L476RG, BMP280, Power Resistor, Fan, MOSFETs, LCD 2x16.
* **Software:** STM32CubeIDE, Python 3.x.

### Installation
1.  **Firmware:**
    * Open the project in STM32CubeIDE or build using CMake.
    * Flash the target MCU.
2.  **PC App:**
    * Install dependencies: `pip install pyserial matplotlib tk`
    * Run the script: `python monitor.py`

---

## 👤 Author
**Michał Błotniak**
*Poznan University of Technology*
