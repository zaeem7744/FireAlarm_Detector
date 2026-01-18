/*************************************************************
  Blynk is a platform with iOS and Android apps to control
  ESP32, Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build mobile and web interfaces for any
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: https://www.blynk.io
    Sketch generator:           https://examples.blynk.cc
    Blynk community:            https://community.blynk.cc
    Follow us:                  https://www.fb.com/blynkapp
                                https://twitter.com/blynk_app

  Blynk library is licensed under MIT license
 *************************************************************
  Blynk.Edgent implements:
  - Blynk.Inject - Dynamic WiFi credentials provisioning
  - Blynk.Air    - Over The Air firmware updates
  - Device state indication using a physical LED
  - Credentials reset using a physical Button
 *************************************************************/

/* Fill in information from your Blynk Template here */
/* Read more: https://bit.ly/BlynkInject */
#define BLYNK_TEMPLATE_ID           "TMPL4DMsnssOc"
#define BLYNK_TEMPLATE_NAME         "FireAlarm Detector Dashboard"
// NOTE: With Blynk.Edgent, the Auth Token is provisioned dynamically from the app,
// so BLYNK_AUTH_TOKEN must NOT be defined as a macro here.
 
#define BLYNK_FIRMWARE_VERSION        "0.1.0"
 
#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG
 
#define APP_DEBUG
 
// Using custom board configuration in Settings.h for ESP32-S3 Super Mini
//#define USE_ESP32_DEV_MODULE
//#define USE_ESP32C3_DEV_MODULE
//#define USE_ESP32S2_DEV_KIT
//#define USE_WROVER_BOARD
//#define USE_TTGO_T7
//#define USE_TTGO_T7_S3
//#define USE_TTGO_T_OI
 
#include "BlynkEdgent.h"
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#if defined(BOARD_LED_PIN_WS2812)
#include <Adafruit_NeoPixel.h>
extern Adafruit_NeoPixel rgb;   // Defined in Indicator.h
#endif

// OLED configuration (SSD1306 I2C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool displayReady = false;
bool g_ledOn = false;  // current logical state of the WS2812 strip from Blynk

// Forward declaration
void updateOledStatus();

void setup()
{
  Serial.begin(115200);
  delay(100);

  // Initialize I2C for OLED
  Wire.begin(7, 6);  // SDA = GPIO7, SCL = GPIO6

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    // If OLED init fails, continue without blocking Blynk
    displayReady = false;
  } else {
    displayReady = true;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("FireAlarm Edgent");
    display.println("Booting...");
    display.display();
  }
 
  BlynkEdgent.begin();

#if defined(BOARD_LED_PIN_WS2812)
  // Simple boot test pattern on WS2812 strip to verify wiring (very dim to save power)
  for (uint16_t i = 0; i < BOARD_NEOPIXEL_COUNT; i++) {
    rgb.setPixelColor(i, RGB(0x05, 0x00, 0x00)); // very dim red across whole strip
  }
  rgb.show();
  delay(300);
  for (uint16_t i = 0; i < BOARD_NEOPIXEL_COUNT; i++) {
    rgb.setPixelColor(i, RGB(0x00, 0x00, 0x00)); // off
  }
  rgb.show();
#endif

  // Periodic OLED status update
  extern BlynkTimer edgentTimer;
  edgentTimer.setInterval(1000L, updateOledStatus);
}

// Blynk virtual pin V1: control the whole WS2812 strip
BLYNK_WRITE(V1)
{
#if defined(BOARD_LED_PIN_WS2812)
  int value = param.asInt();
  bool on = (value != 0);
  g_ledOn = on;

  uint32_t color = on ? RGB(0xFF, 0xFF, 0xFF) : RGB(0x00, 0x00, 0x00);
  for (uint16_t i = 0; i < BOARD_NEOPIXEL_COUNT; i++) {
    rgb.setPixelColor(i, color);
  }
  rgb.show();
#endif
}

void updateOledStatus()
{
  if (!displayReady) {
    // If display not initialized, do nothing
    return;
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("FireAlarm Edgent");

  // Show basic Blynk / WiFi state info
  display.print("State: ");
  switch (BlynkState::get()) {
    case MODE_WAIT_CONFIG:       display.println("WAIT_CFG");  break;
    case MODE_CONFIGURING:       display.println("CONFIG");    break;
    case MODE_CONNECTING_NET:    display.println("NET");       break;
    case MODE_CONNECTING_CLOUD:  display.println("CLOUD");     break;
    case MODE_RUNNING:           display.println("RUNNING");   break;
    case MODE_OTA_UPGRADE:       display.println("OTA");       break;
    case MODE_SWITCH_TO_STA:     display.println("SWITCH_STA");break;
    case MODE_RESET_CONFIG:      display.println("RESET_CFG"); break;
    default:                     display.println("OTHER");     break;
  }

  display.print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    display.println("OK");

    // Draw a simple RSSI bar graph (0-4 bars) in the top-right corner
    int32_t rssi = WiFi.RSSI();  // dBm
    uint8_t bars = 0;
    if      (rssi > -55) bars = 4;   // excellent
    else if (rssi > -65) bars = 3;   // good
    else if (rssi > -75) bars = 2;   // fair
    else if (rssi > -85) bars = 1;   // weak
    else                  bars = 0;   // very poor / unknown

    const int barW  = 3;
    const int barS  = 1;
    const int barH1 = 3;
    const int baseY = 10;           // aligned with first text line
    const int baseX = 128 - (4 * barW + 3 * barS) - 2;  // a bit inset from right edge

    for (uint8_t i = 0; i < 4; i++) {
      int x = baseX + i * (barW + barS);
      int h = barH1 + i * 2;   // increasing height
      int y = baseY - h;
      if (i < bars) {
        display.fillRect(x, y, barW, h, SSD1306_WHITE);
      } else {
        display.drawRect(x, y, barW, h, SSD1306_WHITE);
      }
    }

    display.setCursor(0, 32);
    display.print("IP: ");
    display.println(WiFi.localIP());
  } else {
    display.println("DISCONNECTED");
  }

  // Show LED strip logical state from Blynk
  display.setCursor(0, 48);
  display.print("LED: ");
  display.println(g_ledOn ? "ON" : "OFF");

  display.display();
}
 
void loop() {
  BlynkEdgent.run();
}

