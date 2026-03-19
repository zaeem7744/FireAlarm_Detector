# 🔥 Fire Alarm Detection & Alert System — ESP32 + AI/ML

![ESP32](https://img.shields.io/badge/ESP32-E7352C?style=flat-square&logo=espressif&logoColor=white)
![TensorFlow Lite](https://img.shields.io/badge/TensorFlow_Lite-FF6F00?style=flat-square&logo=tensorflow&logoColor=white)
![C++](https://img.shields.io/badge/C/C++-00599C?style=flat-square&logo=cplusplus&logoColor=white)
![Blynk IoT](https://img.shields.io/badge/Blynk_IoT-00C48D?style=flat-square)
![WiFi](https://img.shields.io/badge/Wi--Fi-4285F4?style=flat-square&logo=wifi&logoColor=white)

AI-powered IoT device that uses **on-device machine learning** to detect fire alarm sound patterns in real-time. Features visual alerts, cloud connectivity, and mobile push notifications.

---

## 📸 Project Images

<p>
  <img src="Image%201.jpg" width="45%" />
  <img src="Image%202.jpg" width="45%" />
</p>

---

## 🔧 Features

- **On-Device ML Inference** — TensorFlow Lite model running on ESP32 for real-time fire alarm sound pattern recognition
- **AI Confidence Scoring** — Displays detection confidence percentage on OLED screen
- **WS2812B LED Alerts** — Addressable neon LED strip for visual alarm indication
- **OLED Status Display** — Real-time system status, alarm confidence level, and Wi-Fi status
- **Buzzer Alert** — Audible alarm output triggered by ML detection
- **Cloud Connectivity** — Blynk IoT platform integration with mobile app push notifications
- **Event History** — Cloud-stored detection events with timestamps
- **Wi-Fi Provisioning** — Hotspot mode for initial setup without hardcoded credentials
- **Physical Reset Button** — With progress bar animation on OLED
- **Remote LED Control** — Control LED strip color and mode from mobile app

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────┐
│                 ESP32 MCU                    │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │ Sound    │  │ TF Lite  │  │ Blynk    │  │
│  │ Sensor   │→ │ ML Model │→ │ Cloud    │  │
│  └──────────┘  └──────────┘  └──────────┘  │
│       ↓              ↓             ↓        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │  OLED    │  │ WS2812B  │  │  Buzzer  │  │
│  │ Display  │  │ LED Strip│  │  Alert   │  │
│  └──────────┘  └──────────┘  └──────────┘  │
└─────────────────────────────────────────────┘
         ↕                ↕
   ┌──────────┐    ┌──────────┐
   │ Wi-Fi    │    │ Mobile   │
   │Provision │    │   App    │
   └──────────┘    └──────────┘
```

---

## 📂 Project Structure

```
├── Firmware/              # ESP32 firmware source code
├── Image 1.jpg            # Project photo
├── Image 2.jpg            # Project photo
├── BOM.pdf                # Bill of Materials
├── FireAlarm_Detector_User_Manual.md  # User manual
└── IOT Based FireAlarm Detector.pdf   # Technical documentation
```

---

## 🛠️ Tech Stack

- **MCU:** ESP32
- **Language:** C/C++
- **AI/ML:** TensorFlow Lite for Microcontrollers (Edge AI)
- **Cloud:** Blynk IoT Platform
- **Display:** SSD1306 OLED
- **LEDs:** WS2812B (Addressable NeoPixel)
- **Connectivity:** Wi-Fi with provisioning mode

---

## 👤 Author

**Muhammad Zaeem Sarfraz**
- 🔗 [LinkedIn](https://www.linkedin.com/in/zaeemsarfraz7744/)
- 📧 Zaeem.7744@gmail.com
- 🌍 Vaasa, Finland
