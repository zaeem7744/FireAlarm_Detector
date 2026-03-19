# ðŸ”¥ Fire Alarm Detection & Alert System â€” ESP32 + AI/ML

![ESP32](https://img.shields.io/badge/ESP32-E7352C?style=flat-square&logo=espressif&logoColor=white)
![TensorFlow Lite](https://img.shields.io/badge/TensorFlow_Lite-FF6F00?style=flat-square&logo=tensorflow&logoColor=white)
![C++](https://img.shields.io/badge/C/C++-00599C?style=flat-square&logo=cplusplus&logoColor=white)
![Blynk IoT](https://img.shields.io/badge/Blynk_IoT-00C48D?style=flat-square)
![WiFi](https://img.shields.io/badge/Wi--Fi-4285F4?style=flat-square&logo=wifi&logoColor=white)

AI-powered IoT device that uses **on-device machine learning** to detect fire alarm sound patterns in real-time. Features visual alerts, cloud connectivity, and mobile push notifications.

---

## ðŸ“¸ Project Images

<p>
  <img src="Image%201.jpg" width="45%" />
  <img src="Image%202.jpg" width="45%" />
</p>

---

## ðŸ”§ Features

- **On-Device ML Inference** â€” TensorFlow Lite model running on ESP32 for real-time fire alarm sound pattern recognition
- **AI Confidence Scoring** â€” Displays detection confidence percentage on OLED screen
- **WS2812B LED Alerts** â€” Addressable neon LED strip for visual alarm indication
- **OLED Status Display** â€” Real-time system status, alarm confidence level, and Wi-Fi status
- **Buzzer Alert** â€” Audible alarm output triggered by ML detection
- **Cloud Connectivity** â€” Blynk IoT platform integration with mobile app push notifications
- **Event History** â€” Cloud-stored detection events with timestamps
- **Wi-Fi Provisioning** â€” Hotspot mode for initial setup without hardcoded credentials
- **Physical Reset Button** â€” With progress bar animation on OLED
- **Remote LED Control** â€” Control LED strip color and mode from mobile app

---

## ðŸ—ï¸ System Architecture

```
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚                 ESP32 MCU                    â”‚
â”‚  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”  â”‚
â”‚  â”‚ Sound    â”‚  â”‚ TF Lite  â”‚  â”‚ Blynk    â”‚  â”‚
â”‚  â”‚ Sensor   â”‚â†’ â”‚ ML Model â”‚â†’ â”‚ Cloud    â”‚  â”‚
â”‚  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜  â”‚
â”‚       â†“              â†“             â†“        â”‚
â”‚  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”  â”‚
â”‚  â”‚  OLED    â”‚  â”‚ WS2812B  â”‚  â”‚  Buzzer  â”‚  â”‚
â”‚  â”‚ Display  â”‚  â”‚ LED Stripâ”‚  â”‚  Alert   â”‚  â”‚
â”‚  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜  â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
         â†•                â†•
   â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”    â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
   â”‚ Wi-Fi    â”‚    â”‚ Mobile   â”‚
   â”‚Provision â”‚    â”‚   App    â”‚
   â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜    â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
```

---

## ðŸ“‚ Project Structure

```
â”œâ”€â”€ Firmware/              # ESP32 firmware source code
â”œâ”€â”€ Image 1.jpg            # Project photo
â”œâ”€â”€ Image 2.jpg            # Project photo
â”œâ”€â”€ BOM.pdf                # Bill of Materials
â”œâ”€â”€ FireAlarm_Detector_User_Manual.md  # User manual
â””â”€â”€ IOT Based FireAlarm Detector.pdf   # Technical documentation
```

---

## ðŸ› ï¸ Tech Stack

- **MCU:** ESP32
- **Language:** C/C++
- **AI/ML:** TensorFlow Lite for Microcontrollers (Edge AI)
- **Cloud:** Blynk IoT Platform
- **Display:** SSD1306 OLED
- **LEDs:** WS2812B (Addressable NeoPixel)
- **Connectivity:** Wi-Fi with provisioning mode

---

## ðŸ‘¤ Author

**Muhammad Zaeem Sarfraz**
- ðŸ”— [LinkedIn](https://www.linkedin.com/in/zaeemsarfraz7744/)
- ðŸ“§ Zaeem.7744@gmail.com
- ðŸŒ Vaasa, Finland
