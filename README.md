# Greenhouse Plant Monitoring System

## Overview

This project presents the design and implementation of a smart greenhouse monitoring system using embedded systems and IoT technologies.

The system continuously monitors important environmental parameters affecting plant growth, including air temperature, air humidity, soil moisture, and light intensity. Sensor data are processed by an STM32 microcontroller, displayed locally on an OLED screen, and transmitted to Firebase through an ESP8266 WiFi module for real-time remote monitoring.

Users can monitor environmental conditions and control devices such as cooling fans and water pumps through a mobile application. The system supports both manual and automatic operating modes, providing a practical solution for smart agriculture and greenhouse management.

---

## Features

- Real-time monitoring of: Air temperature, Air humidity, Soil moisture and Light intensity.
- Local display using OLED screen.
- Cloud data storage using Firebase.
- Mobile application for remote monitoring.
- Manual and automatic operating modes
- Automatic environmental control: Cooling fan control and Water pump control.
- Alarm system using buzzer.
- WiFi connectivity via ESP8266.
- Custom PCB designed using Altium Designer.

---

## System Architecture

### Main Controller
- STM32F103C8T6.

### Sensors
- DHT22: Air temperature and Air humidity.
- SMS-V2: Soil moisture.
- BH1750: Light intensity

### Communication
- ESP8266 WiFi Module.
- UART Communication.
- Firebase Realtime Database.

### Output Devices
- OLED SSD1306 Display.
- Buzzer.
- Relay Module.
- Cooling Fan.
- Mini Water Pump.

---

## Operating Modes

### Automatic Mode
The system automatically controls the cooling fan and water pump according to predefined environmental thresholds.
Examples:
- Temperature > 30°C → Turn on cooling fan.
- Soil moisture < threshold → Turn on water pump.
- Abnormal environmental conditions → Activate buzzer and send alerts.

### Manual Mode
Users can remotely control devices through the mobile application using Firebase communication.

---

## Mobile Application
The mobile application provides:
- Real-time environmental monitoring.
- Device status monitoring.
- Manual control of fan and pump.
- Automatic mode selection.
- WiFi configuration interface.

---

## Hardware Components

| Component     | Function                       |
| ------------- | ------------------------------ |
| STM32F103C8T6 | Main controller                |
| ESP8266       | WiFi communication             |
| DHT22         | Temperature & humidity sensing |
| SMS-V2        | Soil moisture sensing          |
| BH1750        | Light intensity sensing        |
| OLED SSD1306  | Data display                   |
| Relay Module  | Device switching               |
| Buzzer        | Alarm notification             |
| Cooling Fan   | Temperature control            |
| Water Pump    | Irrigation system              |

---

## Software Tools

### Firmware Development
- STM32CubeIDE.
- HAL Library.

### IoT & Cloud
- Firebase Realtime Database.
- ESP8266 Firmware.

### Mobile Application
- Thunkable.

### PCB Design
- Altium Designer.

---

## Results

The developed system successfully demonstrates:
- Stable environmental monitoring.
- Accurate sensor measurements.
- Real-time Firebase communication.
- Remote monitoring via mobile application.
- Automatic irrigation control.
- Automatic cooling control.
- Audible warning system.
- Reliable long-term operation.

The system provides a low-cost and practical solution for greenhouse monitoring and smart agriculture applications.

---

## Author
**Ngô Diễm Quỳnh**
Student ID: 2212887

Department of Electronics Engineering

Ho Chi Minh City University of Technology (HCMUT)

Vietnam National University Ho Chi Minh City

Supervisor: **Dr. Nguyễn Lý Thiên Trường**

---

## License

This repository is published for academic and educational purposes.

Please contact the author before using the materials for commercial applications.
