# :fire: Fire Alarm Detector -- AI-Powered Sound Detection IoT Device

![ESP32](https://img.shields.io/badge/ESP32-E7352C?style=flat-square&logo=espressif&logoColor=white)
![TensorFlow](https://img.shields.io/badge/TensorFlow_Lite-FF6F00?style=flat-square&logo=tensorflow&logoColor=white)
![C++](https://img.shields.io/badge/C/C++-00599C?style=flat-square&logo=cplusplus&logoColor=white)
![Blynk IoT](https://img.shields.io/badge/Blynk_IoT-23C48E?style=flat-square&logo=blynk&logoColor=white)
![WiFi](https://img.shields.io/badge/Wi--Fi-4285F4?style=flat-square&logo=wifi&logoColor=white)

An ESP32-based IoT device that uses **on-device AI/ML audio classification** to detect fire alarm siren patterns in real-time. Features addressable LED strip alerts, OLED display with live confidence metrics, haptic buzzer, cloud notifications via Blynk IoT, and OTA firmware updates.

---

## :camera: Hardware

### Device Assembly
![Fire Alarm Detector - Assembly](Image%201.jpg)

### Internal Electronics
![Fire Alarm Detector - Electronics](Image%202.jpg)

---

## :zap: Key Highlights

- **On-device AI audio classification** -- TensorFlow Lite model trained on fire alarm siren patterns
- **Real-time confidence display** -- OLED shows `Alarm: XX%` and `Background: YY%` probabilities
- **Multi-sensory alerts** -- Red LED strip blinking + buzzer beeps + mobile push notifications
- **Blynk IoT cloud integration** -- Remote monitoring, notifications, Wi-Fi signal widget, LED control
- **OTA firmware updates** -- Blynk Edgent enables wireless firmware deployment
- **Wi-Fi provisioning** -- Hotspot-based setup, no hardcoded credentials
- **Plug-and-play operation** -- Boot rainbow, auto-connect, continuous monitoring

---

## :wrench: Features

### AI/ML Audio Detection
- TensorFlow Lite Micro model running directly on ESP32
- Trained on fire alarm siren audio patterns with custom training data
- Outputs real-time probability scores: **Alarm confidence** vs **Background noise**
- Threshold-based triggering with configurable sensitivity

### Alert System (10-Second Alarm Window)
- **LED Strip** -- All addressable LEDs blink red (500ms on/off) for 10 seconds
- **Fire Status LED** -- Dedicated indicator LED turns ON automatically
- **Buzzer** -- Fast beep pattern for approximately 5 seconds
- **Cloud Notification** -- Push notification with message: `Fire alarm detected, confidence XX%`

### OLED Display
- Wi-Fi signal strength bars (top-right corner)
- System state: `ALERT` / `Monitoring` / `Idle (No Wi-Fi)` / `Starting...`
- Wi-Fi connection status
- Live alarm and background noise confidence percentages

### Wi-Fi & Cloud
- **Hotspot-based provisioning** -- Device creates AP for initial setup via Blynk app
- **Automatic reconnection** -- Reconnects to saved network after disconnection
- **Wi-Fi signal monitoring** -- Signal strength (0-100%) reported to app dashboard
- **Remote LED control** -- Manual fire status LED toggle via Blynk virtual pin

### Physical Controls
- **Reset button**: Short press = visual feedback, Long press (~10s) = factory reset (clears Wi-Fi + cloud credentials)
- Progress bar on OLED during reset confirmation

---

## :file_folder: Project Structure

```
FireAlarm_Detector/
|-- Firmware/
|   |-- Audio_Model/              # TensorFlow Lite audio classification model
|   |-- FireAlarm_Detector/       # Main ESP32 firmware (PlatformIO)
|   |-- FireAlarm_Edgent_ESP32/   # Blynk Edgent integration (OTA + provisioning)
|   +-- Training_Data/            # ML model training audio samples
|-- FireAlarm_Detector_User_Manual.md   # Comprehensive user guide
|-- IOT Based FireAlarm Detector.pdf    # Project documentation
|-- BOM.pdf                       # Bill of Materials
|-- Image 1.jpg                   # Device assembly photo
+-- Image 2.jpg                   # Internal electronics photo
```

---

## :hammer_and_wrench: Tech Stack

| Component | Technology |
|-----------|-----------|
| **MCU** | ESP32 (Espressif Systems) |
| **Language** | C/C++ (PlatformIO / Arduino) |
| **AI/ML** | TensorFlow Lite Micro -- on-device audio classification |
| **Cloud** | Blynk IoT (notifications, OTA, remote control) |
| **Display** | SSD1306 OLED (I2C) |
| **LEDs** | Addressable RGB LED strip (WS2812B) |
| **Audio** | Sound sensor + ML inference pipeline |
| **Communication** | Wi-Fi (802.11 b/g/n) |
| **Provisioning** | Blynk Edgent (hotspot-based) |
| **Alerts** | LED strip + dedicated LED + buzzer + push notifications |

---

## :gear: System Behavior Summary

| State | LED Strip | Fire LED | OLED | Action |
|-------|-----------|----------|------|--------|
| **Normal** | Off | Off (or manual) | `Monitoring`, `Alarm: ~0%` | Continuous listening |
| **Setup Mode** | Blue blink (10s) | -- | Setup instructions | Hotspot active for provisioning |
| **Wi-Fi Lost** | Blue blink (5s) | -- | `Disconnected` | Auto-reconnect in background |
| **Fire Alarm** | Red blink (10s) | ON (10s) | `ALERT`, high confidence | Buzzer + cloud notification |

---

## :bust_in_silhouette: Author

**Muhammad Zaeem Sarfraz** -- Electronics & IoT Hardware Engineer

- :link: [LinkedIn](https://www.linkedin.com/in/zaeemsarfraz7744/)
- :email: Zaeem.7744@gmail.com
- :earth_africa: Vaasa, Finland
