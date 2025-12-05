# LiveVote: Secure RFID-Based Tangible Voting System 🗳️

**LiveVote** is an IoT-based voting solution designed to eliminate voter fraud using RFID authentication. It features a tangible interface for casting votes and a real-time web dashboard for monitoring election results.

## 🌟 Features
* **RFID Authentication:** Ensures "One Person, One Vote" using unique RFID tags.
* **Duplicate Prevention:** Automatically blocks cards that have already voted.
* **Tangible Interface:** Physical buttons for candidate selection (BNP, NCP, JMT).
* **Real-Time Web Server:** The ESP32 hosts a local website displaying live vote counts.
* **Visual Feedback:** LCD display and LED indicators for Approved/Denied status.

## 🛠️ Hardware Requirements
* **Microcontroller:** ESP32 Development Board
* **RFID Module:** MFRC522 (RC522)
* **Display:** 16x2 LCD with I2C Backpack
* **Inputs:** 4x Push Buttons (3 Candidates + 1 Stop Button)
* **Indicators:** 3x LEDs (Status indicators per candidate)

## 🔌 Pin Configuration
| Component | ESP32 Pin |
| :--- | :--- |
| **RFID SDA (SS)** | GPIO 5 |
| **RFID SCK** | GPIO 18 |
| **RFID MOSI** | GPIO 23 |
| **RFID MISO** | GPIO 19 |
| **LCD SDA** | GPIO 21 |
| **LCD SCL** | GPIO 22 |
| **Button BNP** | GPIO 13 |
| **Button NCP** | GPIO 14 |
| **Button JMT** | GPIO 27 |
| **Button STOP** | GPIO 26 |

