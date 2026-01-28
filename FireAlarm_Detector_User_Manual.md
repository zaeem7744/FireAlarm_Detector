# Fire Alarm Detector – User Guide

This device listens for fire alarm sounds and alerts you using lights, a buzzer, and notifications via a mobile app.
It also shows live noise and alarm confidence on a small display.

This guide explains:

- What each light pattern means
- How to power on and set up Wi‑Fi
- How to interpret the OLED screen
- How the system behaves during a fire alarm
- How to use the extra fire‑status LED and Wi‑Fi‑signal widget

---

## 1. Hardware overview

Your Fire Alarm Detector includes:

- **Sound sensor + AI model**  
  Listens for fire alarm siren patterns and estimates how likely there is an alarm present.

- **Neon light strip (addressable LEDs)**  
  Used for:
  - Startup animation
  - Wi‑Fi / status indications
  - Fire alarm indication (red blinking)

- **Fire status LED (separate single LED)**  
  - Used as a clear “Fire Alarm Active” indicator.  
  - Can also be controlled manually from the mobile app.

- **Buzzer**  
  Emits a beeping pattern during a fire alarm event.

- **Reset button**  
  - Short or medium press: shows visual feedback only.  
  - Long press (about 10 seconds): erases Wi‑Fi and cloud credentials and puts the unit into setup mode.

- **OLED display**  
  Shows Wi‑Fi status, system mode, and real‑time alarm/noise confidence values.

- **Companion mobile app & cloud dashboard**  
  - Used to add the device, configure Wi‑Fi, and view notifications & history.  
  - Provides widgets such as Wi‑Fi signal strength and manual LED controls.

---

## 2. Power‑on behavior & light indications

### 2.1. Boot animation (every power‑on)

1. **Rainbow sweep (~6 seconds)**  
   - The neon strip shows a flowing rainbow pattern.  
   - Confirms the device is powered, the LED strip is connected, and the firmware is running.  
   - During this time, Wi‑Fi, audio capture, and cloud connection are starting in the background.

2. After the rainbow finishes, the strip turns **off** and the device switches to its normal indication logic.

---

## 3. Wi‑Fi & connection states (neon strip)

The neon strip uses **color + blinking duration** to show connection states. All blinks are about **500 ms ON / 500 ms OFF**.

### 3.1. Setup mode (no Wi‑Fi configured at power‑on)

- **Pattern:** Blue blinking on the strip for **10 seconds**, then off.  
- **When it happens:**
  - After the boot rainbow, if the device has **no saved Wi‑Fi / cloud account**, it enters setup (hotspot) mode.
- **What it means:**
  - The device has created its own Wi‑Fi hotspot for onboarding.  
  - Use the companion app’s *Add Device* flow to connect to this hotspot and provide your home Wi‑Fi details.

After 10 seconds of blue blink, the LEDs go off, but the device remains in setup mode until you complete configuration or reboot.

### 3.2. Successful connection to cloud (normal monitoring)

- **Pattern:** Light‑blue (teal) blinking for **5 seconds**, then off.  
- **When it happens:**
  - After the device has successfully connected to your Wi‑Fi network **and** to the cloud service for the first time after boot.
- **What it means:**
  - The device is now fully online and monitoring for fire alarms.  
  - After the 5‑second indication, the strip turns off (idle state).

### 3.3. Wi‑Fi lost during normal operation

- **Pattern:** Blue blinking for **5 seconds**, then off.  
- **When it happens:**
  - The device was connected and running normally, then lost its Wi‑Fi connection.
- **What it means:**
  - The unit has dropped off the network.  
  - After 5 seconds of blue blinking, the LEDs turn off; the device continues trying to reconnect in the background.

> **Note:** When the strip is off in normal monitoring mode, this is expected. It indicates that everything is fine and the detector is quietly listening.

---

## 4. Fire alarm indication

When the AI model detects a fire alarm pattern with high confidence:

1. **Neon strip**  
   - All LEDs blink **red** at about **500 ms ON / 500 ms OFF** for **10 seconds**.  
   - This gives a clear visual alarm across the entire strip.

2. **Fire status LED (separate LED)**  
   - Turns **ON** automatically for **10 seconds** at the same time as the red strip blinking.  
   - This override has priority over manual control during the alarm window.

3. **Buzzer**  
   - Emits fast beeps for **about 5 seconds** to provide an audible alert.

4. **Notifications & history (via mobile app)**  
   - A cloud event is created with a message such as:  
     `Fire alarm detected, confidence XX%`.  
   - Depending on your app notification settings, you may receive a push notification or email, and the event is recorded in the device’s event history.

After the 10‑second visual alarm window ends, both the neon strip and the Fire status LED return to their non‑alarm behavior (usually off), and the device continues monitoring.

---

## 5. Fire status LED (physical LED) behavior

The Fire status LED behaves as follows:

1. **During a fire alarm:**
   - The LED turns **ON automatically** for **10 seconds**.  
   - Manual control from the app is temporarily ignored during this override.

2. **Outside an alarm:**
   - The LED is controlled by a **Fire LED button** in the mobile app (virtual control channel V0).  
   - Turning that control ON keeps the LED lit steadily; turning it OFF makes it dark.

This LED is useful as a clear “Fire detected” indicator that remains visible even if the neon strip is not in your direct line of sight.

---

## 6. OLED screen – what it shows

The OLED gives a compact summary of system state.

### 6.1. Layout

- **Top line – Wi‑Fi signal bars**  
  A small bar graph in the top‑right corner shows approximate Wi‑Fi signal quality:
  - More filled bars = stronger signal.

- **Second line – Device title**  
  A boxed title: `Fire Alarm Detector`.

- **System state row** (around y=24):

  - Label: `System:` followed by one of:
    - `ALERT` – Fire alarm confidence is high (above threshold).  
    - `Monitoring` – Device is running normally and connected.  
    - `Idle (No Wi‑Fi)` – No network connection; still listening locally.  
    - `Starting...` – Booting up / not yet in full monitoring mode.

- **Wi‑Fi status row** (around y=32):

  - Shows `Wi‑Fi: Connected` or `Wi‑Fi: Disconnected`.

- **Noise and alarm confidence row** (around y=44):

  - `Alarm: XX%` – Current probability the sound is a fire alarm.  
  - `Bg: YY%` – Current probability the sound is normal background noise.

These percentages come directly from the on‑device AI model. In normal conditions, `Alarm` should hover near 0% and `Bg` near 100%. During an alarm sound, you’ll see `Alarm` jump up.

### 6.2. Reset & setup messages

When you press and hold the reset button, the OLED temporarily switches to show button‑related messages instead of the monitoring UI:

- **Short press:**  
  - Message: `Press & hold to reset Wi‑Fi`.

- **Medium hold (after a few seconds):**  
  - Message: `Hold to reset Wi‑Fi` followed by `Confirming...` with animated dots.

- **Long hold (around 10 seconds):**  
  - Message: `Resetting Wi-Fi Settings` plus a **progress bar** on the next line.  
  - When the bar completes, the stored Wi‑Fi and cloud account information is erased, and the device will go back to setup/hotspot mode on the next cycle.

During these screens, the normal monitoring UI is paused; once you release the button or the reset completes, the display returns to showing System / Wi‑Fi / Alarm / Bg information.

---

## 7. Resetting Wi‑Fi and cloud account

You can perform a full Wi‑Fi + account reset using the physical button:

1. **Power the device.**
2. Press and hold the **reset button**:
   - After a few seconds you’ll see `Hold to reset Wi-Fi` and `Confirming...` on the OLED.  
   - Keep holding until you see `Resetting Wi-Fi Settings` and the progress bar starts moving.
3. Continue holding until the progress bar finishes.  
   Release the button.

What this does:

- Clears the stored Wi‑Fi network and cloud credentials from internal memory.  
- Puts the device into setup mode on the next cycle.

After this, the next time the unit boots, you will see:

- Boot rainbow (~6 s), then
- Blue blinking on the neon strip for 10 seconds, indicating it is ready to be added again via the mobile app.

---

## 8. Connecting the device to your Wi‑Fi and app

1. **First‑time power‑up or after reset:**
   - Power on the device.  
   - Wait for the boot rainbow to finish.  
   - If no Wi‑Fi is configured, you will see the blue 10‑second blink on the strip.

2. **From the mobile app:**
   - Open the app and create a new device from the appropriate product/dashboard.  
   - Follow the app’s instructions to connect your phone to the temporary hotspot broadcast by the detector.  
   - Select your home Wi‑Fi network and enter its password.

3. **Device connects:**
   - The detector will reboot or switch to station mode, connect to your router, then to the cloud.  
   - When it successfully connects, the strip will blink light‑blue for 5 seconds.  
   - In the app, the device should appear as **online**.

Once online, the device continuously listens for fire alarms and updates its status.

---

## 9. Wi‑Fi signal strength widget in the app

The firmware periodically sends Wi‑Fi signal strength (0–100%) to a **virtual Wi‑Fi strength gauge** in the app.

- When the device is close to your router, the reading should be high (e.g. 80–100%).
- If the value is consistently low (e.g. below 30%), consider moving the device or your router, or adding a range extender to ensure reliable communication for notifications.

This does not affect the detector’s core listening ability, but it does affect how reliably it can send alerts to your phone.

---

## 10. Manual control of Fire status LED from the app

In addition to its automatic behavior during a fire alarm, the Fire status LED can be controlled manually from the app:

- The app provides a **Fire LED button** (mapped internally to a virtual control channel).  
- When this button is ON, the physical LED turns on steadily.  
- When OFF, the LED is off.

> **Note:** During an active fire alarm, the automatic override has priority. The LED will stay ON for the 10‑second alarm window even if the manual control is OFF. After the window ends, control returns to the manual setting.

---

## 11. Normal vs. alarm conditions – quick reference

- **Normal idle:**
  - Neon strip: off.  
  - Fire status LED: off (unless manually turned on).  
  - OLED: `System: Monitoring`, `Wi-Fi: Connected`, `Alarm: 0–5%`, `Bg: 95–100%`.

- **Setup (hotspot) mode:**
  - After rainbow: blue blink on strip for 10 s.  
  - OLED: `Setup mode`, “Connect phone to …”.

- **Wi‑Fi lost:**
  - Blue blink on strip for 5 s.  
  - OLED: `Wi-Fi: Disconnected` and `System: Idle (No Wi-Fi)`.

- **Fire alarm event:**
  - Neon strip: red blink for 10 s.  
  - Fire status LED: ON for 10 s.  
  - Buzzer: fast beeps (~5 s).  
  - OLED: `System: ALERT`, `Alarm: high %`.  
  - App / cloud: notification + event entry (if enabled).

If the behavior you see differs from this guide, check your power supply, Wi‑Fi signal, and app configuration, or re‑run the Wi‑Fi/account reset procedure.

---

## 12. Safety note

This device is intended as an **aid** for detecting audio patterns similar to fire alarms, not as a certified life‑safety system.

- Always install and maintain proper smoke detectors and fire alarms according to local regulations.  
- Treat any alert from this device seriously, but do not rely on it as your only means of fire detection.

If you have questions about wiring, placement, or behavior, refer back to this guide or consult the project documentation.
