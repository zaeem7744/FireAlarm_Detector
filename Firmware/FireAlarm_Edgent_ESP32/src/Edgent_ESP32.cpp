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
#include <math.h>

#if defined(BOARD_LED_PIN_WS2812)
#include <Adafruit_NeoPixel.h>
extern Adafruit_NeoPixel rgb;   // Defined in Indicator.h
#endif

// Edge Impulse fire alarm inference (merged from inference_firealarm.cpp)
#include <FireAlarm_Detector_ML_inferencing.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"

/** Audio buffers, pointers and selectors */
typedef struct {
    signed short *buffers[2];
    unsigned char buf_select;
    unsigned char buf_ready;
    unsigned int buf_count;
    unsigned int n_samples;
} inference_t;

static inference_t inference;
static const uint32_t sample_buffer_size = 2048;
static signed short sampleBuffer[sample_buffer_size];
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
static bool record_status = true;

// Buzzer configuration
static const int BUZZER_PIN = 8;           // GPIO8 on ESP32-S3 Super Mini
// Trigger when Alarm confidence >= 70%
static const float ALARM_THRESHOLD = 0.7f;
static unsigned long alarm_start_ms = 0;   // when current alarm burst started

// Latest probabilities for external use (OLED, etc.)
static float g_noise_prob_ml = 0.0f;
static float g_alarm_prob_ml = 0.0f;

// Forward declarations for EI inference internals
static bool microphone_inference_start(uint32_t n_samples);
static bool microphone_inference_record(void);
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr);
static void microphone_inference_end(void);
static int i2s_init(uint32_t sampling_rate);
static int i2s_deinit(void);
static void audio_inference_callback(uint32_t n_bytes);
static void capture_samples(void* arg);

// Public-style helpers used later in this file
static void firealarm_inference_init();
static void firealarm_inference_step();
static float firealarm_get_noise_prob();
static float firealarm_get_alarm_prob();

// OLED configuration (SSD1306 I2C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool displayReady = false;
bool g_ledOn = false;  // current logical state of the WS2812 strip from Blynk

// LED animation state
enum LedAnimMode {
  LED_ANIM_NONE,
  LED_ANIM_BOOT_RAINBOW,
  LED_ANIM_BOOT_HEARTBEAT,
  LED_ANIM_WIFI_DISCONNECTED,
  LED_ANIM_FIRE_ALERT,
  LED_ANIM_WIFI_CONNECTED_OK   // solid light blue for 10s when Wi-Fi connects
};

struct LedAnimState {
  LedAnimMode mode;
  unsigned long startMs;
  bool active;
};

LedAnimState g_ledAnim = { LED_ANIM_NONE, 0, false };
static unsigned long g_ledAnimDurationMs = 0;     // duration for current g_ledAnim.mode
static bool         g_initialStatusShown = false; // one-time status after rainbow
static bool         g_ledAnimBlink       = false; // whether current Wi-Fi anim should blink
static unsigned long g_ledAnimBlinkStepMs = 0;    // blink speed for Wi-Fi anim

enum BootPhase {
  BOOT_PHASE_RAINBOW,
  BOOT_PHASE_HEARTBEAT,
  BOOT_PHASE_DONE
};

static BootPhase g_bootPhase = BOOT_PHASE_DONE;  // Boot rainbow handled blocking in setup()
static unsigned long g_bootPhaseStart = 0;

bool g_prevWifiConnected = false;
bool g_prevAlarmHigh     = false;
State g_prevBlynkState   = MODE_MAX_VALUE;  // track state transitions for LED timings

// Forward declarations
void updateOledStatus();
static void updateLedAnimation();
void ledAnimationLoop();
static uint32_t colorWheel(uint8_t pos);
static void fillStripColor(uint8_t r, uint8_t g, uint8_t b);
static void renderRainbowChase(unsigned long now, uint8_t stepSize);
static void renderFireChase(unsigned long now);
static void renderHeartbeat(unsigned long now, uint32_t baseColor, uint16_t periodMs);
static void renderWifiDisconnected(unsigned long now);
static void renderFastBlink(unsigned long now, uint8_t r, uint8_t g, uint8_t b, unsigned long stepMs);

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
    // Top row reserved for Wi-Fi icon; title on second row
    display.setCursor(0, 10);
    display.println("Fire Alarm Detector");
    display.println("Booting...");
    display.display();
  }
 
  BlynkEdgent.begin();

  // Initialize Edge Impulse-based fire alarm inference
  firealarm_inference_init();

  // Mark boot phase as rainbow; handled fully here in setup()
  g_bootPhase = BOOT_PHASE_RAINBOW;

#if defined(BOARD_LED_PIN_WS2812)
  // Guaranteed rainbow boot animation (~6s), independent of Blynk/Wi-Fi state
  rgb.begin();
  rgb.setBrightness(BOARD_LED_BRIGHTNESS);
  unsigned long rbStart = millis();
  uint16_t offset = 0;
  while (millis() - rbStart < 6000UL) {
    uint16_t count = BOARD_NEOPIXEL_COUNT;
    for (uint16_t i = 0; i < count; i++) {
      uint8_t phase = (uint8_t)(((i + offset) * 256) / count);
      rgb.setPixelColor(i, colorWheel(phase));
    }
    rgb.show();
    offset++;
    delay(40);  // slower motion, ~25 FPS
  }
  // Clear after rainbow; runtime animations take over in loop()
  for (uint16_t i = 0; i < BOARD_NEOPIXEL_COUNT; i++) {
    rgb.setPixelColor(i, 0);
  }
  rgb.show();
  // After blocking boot rainbow, mark boot phase done so loop() logic never re-runs it
  g_bootPhase = BOOT_PHASE_DONE;
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
  // When ON, override animations with fast rainbow; when OFF, turn strip off
  g_ledOn = on;
  if (!on) {
    fillStripColor(0, 0, 0);
  }
#endif
}

void updateOledStatus()
{
  if (!displayReady) {
    // If display not initialized, do nothing
    return;
  }

  display.clearDisplay();

  display.clearDisplay();

  // ===== Top line: Wi-Fi icon only =====
  uint8_t bars = 0;
  if (WiFi.status() == WL_CONNECTED) {
    int32_t rssi = WiFi.RSSI();  // dBm
    if      (rssi > -55) bars = 4;   // excellent
    else if (rssi > -65) bars = 3;   // good
    else if (rssi > -75) bars = 2;   // fair
    else if (rssi > -85) bars = 1;   // weak
    else                  bars = 0;  // very poor
  }
  const int barW  = 3;
  const int barS  = 1;
  const int barH1 = 3;
  const int baseY = 8;            // slightly below top border
  const int baseX = 128 - (4 * barW + 3 * barS) - 2;
  for (uint8_t i = 0; i < 4; i++) {
    int x = baseX + i * (barW + barS);
    int h = barH1 + i * 2;
    int y = baseY - h;
    if (i < bars) {
      display.fillRect(x, y, barW, h, SSD1306_WHITE);
    } else {
      display.drawRect(x, y, barW, h, SSD1306_WHITE);
    }
  }

  // ===== Second line: fixed device title in a box =====
  display.drawRect(0, 10, 128, 10, SSD1306_WHITE);
  display.setCursor(2, 12);
  display.print("Fire Alarm Detector");

  // ===== Reset / debug overlays =====
  State currState = BlynkState::get();
  unsigned long nowMs = millis();
  if (g_buttonPressed) {
    uint32_t held = nowMs - g_buttonPressTime;
    display.setCursor(0, 24);
    if (held > BUTTON_HOLD_TIME_ACTION) {
      display.println("Resetting Wi-Fi Settings");
      // Move progress bar to next line to avoid overlapping text
      display.setCursor(0, 34);
      uint8_t progress = (held / 200) % 12; // up to full width
      int barX = 0;
      int barY = 44;
      int blockW = 8;
      for (uint8_t i = 0; i < 12; i++) {
        int x = barX + i * (blockW + 1);
        if (i <= progress) {
          display.fillRect(x, barY, blockW, 4, SSD1306_WHITE);
        } else {
          display.drawRect(x, barY, blockW, 4, SSD1306_WHITE);
        }
      }
    } else if (held > BUTTON_HOLD_TIME_INDICATION) {
      display.println("Hold to reset Wi-Fi");
      int dots = (held / 400) % 4;
      display.print("Confirming");
      for (int i = 0; i < dots; i++) display.print(".");
    } else {
      display.println("Press & hold to reset Wi-Fi");
    }
    display.display();
    return;
  }

  if (currState == MODE_RESET_CONFIG) {
    display.setCursor(0, 24);
    display.println("Resetting Wi-Fi Settings");
    display.setCursor(0, 34);
    display.println("Please wait...");
    display.display();
    return;
  }

  if (currState == MODE_WAIT_CONFIG) {
    // Device is in configuration (AP) mode after a reset
    display.setCursor(0, 24);
    display.println("Setup mode");
    display.setCursor(0, 34);
    display.println("Connect phone to");
    display.setCursor(0, 44);
    display.println("Blynk Wi-Fi app");
    display.display();
    return;
  }

  // ===== Normal monitoring UI =====

  // System state row
  display.setCursor(0, 24);
  display.print("System: ");
  float alarmProb = firealarm_get_alarm_prob();
  const float ALERT_VISUAL_THRESHOLD = 0.7f;
  if (alarmProb >= ALERT_VISUAL_THRESHOLD) {
    display.println("ALERT");
  } else if (currState == MODE_RUNNING && WiFi.status() == WL_CONNECTED) {
    display.println("Monitoring");
  } else if (WiFi.status() != WL_CONNECTED) {
    display.println("Idle (No Wi-Fi)");
  } else {
    display.println("Starting...");
  }

  // Wi-Fi status text (shifted down to avoid overlap)
  display.setCursor(0, 32);
  display.print("Wi-Fi: ");
  if (WiFi.status() == WL_CONNECTED) {
    display.println("Connected");
  } else {
    display.println("Disconnected");
  }

  // Alarm and background sound percentages (compact)
  float noiseProb = firealarm_get_noise_prob();
  display.setCursor(0, 44);
  display.print("Alarm:");
  int alarmPercent = (int)(alarmProb * 100.0f + 0.5f);
  display.print(alarmPercent);
  display.print("%");

  display.setCursor(64, 44);
  int noisePercent = (int)(noiseProb * 100.0f + 0.5f);
  display.print("Bg:");
  display.print(noisePercent);
  display.print("%");

  display.display();
}
 
void loop() {
  BlynkEdgent.run();
  // Run one step of the Edge Impulse fire alarm inference inside Edgent loop
  firealarm_inference_step();
  // LED animations are driven from app_loop() to work in all Blynk states
}

// ===== LED animation helpers =====

static uint32_t colorWheel(uint8_t pos) {
  if (pos < 85) {
    return rgb.Color(pos * 3, 255 - pos * 3, 0);
  } else if (pos < 170) {
    pos -= 85;
    return rgb.Color(255 - pos * 3, 0, pos * 3);
  } else {
    pos -= 170;
    return rgb.Color(0, pos * 3, 255 - pos * 3);
  }
}

static void fillStripColor(uint8_t r, uint8_t g, uint8_t b) {
#if defined(BOARD_LED_PIN_WS2812)
  for (uint16_t i = 0; i < BOARD_NEOPIXEL_COUNT; i++) {
    rgb.setPixelColor(i, rgb.Color(r, g, b));
  }
  rgb.show();
#endif
}

static void renderRainbowChase(unsigned long now, uint8_t stepSize) {
#if defined(BOARD_LED_PIN_WS2812)
  static uint16_t offset = 0;
  static unsigned long lastStep = 0;
  const unsigned long stepMs = 10;   // faster updates for snappier motion (~100 FPS)
  if (now - lastStep < stepMs) return;
  lastStep = now;

  uint16_t count = BOARD_NEOPIXEL_COUNT;
  for (uint16_t i = 0; i < count; i++) {
    uint8_t phase = (uint8_t)(((i + offset) * 256) / count);
    rgb.setPixelColor(i, colorWheel(phase));
  }
  rgb.show();
  offset = (offset + stepSize) % count;
#endif
}

static void renderFireChase(unsigned long now) {
#if defined(BOARD_LED_PIN_WS2812)
  static uint16_t head = 0;
  static unsigned long lastStep = 0;
  const unsigned long stepMs = 20;   // faster fire chase
  const uint8_t trail = 4;
  if (now - lastStep < stepMs) return;
  lastStep = now;

  uint16_t count = BOARD_NEOPIXEL_COUNT;
  for (uint16_t i = 0; i < count; i++) rgb.setPixelColor(i, 0);
  for (uint8_t t = 0; t < trail; t++) {
    uint16_t idx = (head + t) % count;
    rgb.setPixelColor(idx, rgb.Color(255, 0, 0));
  }
  rgb.show();
  head = (head + 1) % count;
#endif
}

static void renderHeartbeat(unsigned long now, uint32_t baseColor, uint16_t periodMs) {
#if defined(BOARD_LED_PIN_WS2812)
  static unsigned long start = 0;
  if (start == 0) start = now;
  unsigned long t = (now - start) % periodMs;
  float phase = (float)t / (float)periodMs;
  float b = 0.3f + 0.7f * (0.5f - 0.5f * cosf(phase * 2.0f * PI)); // 0.3..1.0

  uint8_t r = (uint8_t)(((baseColor >> 16) & 0xFF) * b);
  uint8_t g = (uint8_t)(((baseColor >> 8)  & 0xFF) * b);
  uint8_t bl= (uint8_t)(( baseColor        & 0xFF) * b);

  fillStripColor(r, g, bl);
#endif
}

static void renderWifiDisconnected(unsigned long now) {
#if defined(BOARD_LED_PIN_WS2812)
  static unsigned long lastStep = 0;
  static bool on = false;
  const unsigned long stepMs = 150;  // faster blink
  if (now - lastStep < stepMs) return;
  lastStep = now;
  on = !on;
  if (on) fillStripColor(0, 0, 32);
  else    fillStripColor(0, 0, 0);
#endif
}

static void renderFastBlink(unsigned long now, uint8_t r, uint8_t g, uint8_t b, unsigned long stepMs) {
#if defined(BOARD_LED_PIN_WS2812)
  static unsigned long lastStep = 0;
  static bool on = false;
  if (now - lastStep < stepMs) return;
  lastStep = now;
  on = !on;
  if (on) fillStripColor(r, g, b);
  else    fillStripColor(0, 0, 0);
#endif
}

static void updateLedAnimation() {
#if defined(BOARD_LED_PIN_WS2812)
  unsigned long now = millis();
  bool wifiConnected = (WiFi.status() == WL_CONNECTED);
  float alarmProb    = firealarm_get_alarm_prob();
  bool alarmHigh     = (alarmProb >= ALARM_THRESHOLD);
  State currState    = BlynkState::get();
  State prevState    = g_prevBlynkState;

  // Manual override from Blynk LED button: fast rainbow until turned off
  if (g_ledOn) {
    renderRainbowChase(now, 4);  // fast but smooth rainbow
    g_prevWifiConnected = wifiConnected;
    g_prevAlarmHigh     = alarmHigh;
    return;
  }

  // Note: boot rainbow is handled fully in setup() as a blocking animation.
  // In the main loop we only handle status/alert animations.

  // State-driven LED animations based on Blynk state transitions

  // 1) Power-on with no Wi-Fi configured: initial entry into MODE_WAIT_CONFIG -> blue blink 10s
  if (prevState == MODE_MAX_VALUE && currState == MODE_WAIT_CONFIG && !alarmHigh) {
    g_ledAnim.mode        = LED_ANIM_WIFI_DISCONNECTED;
    g_ledAnim.startMs     = now;
    g_ledAnimDurationMs   = 10000UL;  // 10 seconds
    g_ledAnimBlink        = true;
    g_ledAnimBlinkStepMs  = 500;      // 500ms on/off
    g_ledAnim.active      = true;
  }

  // 2) Explicit Wi-Fi reset: entering MODE_RESET_CONFIG -> blue blink 2s
  if (currState == MODE_RESET_CONFIG && prevState != MODE_RESET_CONFIG && !alarmHigh) {
    g_ledAnim.mode        = LED_ANIM_WIFI_DISCONNECTED;
    g_ledAnim.startMs     = now;
    g_ledAnimDurationMs   = 2000UL;   // 2 seconds
    g_ledAnimBlink        = true;
    g_ledAnimBlinkStepMs  = 500;      // 500ms on/off
    g_ledAnim.active      = true;
  }

  // 3) Successful connection to Blynk cloud: transition into MODE_RUNNING with Wi-Fi
  if (prevState != MODE_RUNNING && currState == MODE_RUNNING && wifiConnected && !alarmHigh) {
    g_ledAnim.mode        = LED_ANIM_WIFI_CONNECTED_OK;
    g_ledAnim.startMs     = now;
    g_ledAnimDurationMs   = 5000UL;   // 5 seconds
    g_ledAnimBlink        = true;
    g_ledAnimBlinkStepMs  = 500;      // 500ms on/off
    g_ledAnim.active      = true;
  }

  // After state-driven events, handle alerts
  // Fire alert has highest priority: 10s red heartbeat
  if (alarmHigh && !g_prevAlarmHigh) {
    g_ledAnim.mode    = LED_ANIM_FIRE_ALERT;
    g_ledAnim.startMs = now;
    g_ledAnim.active  = true;
  }

  // Wi-Fi disconnected after running: fast blue blink for 5s
  if (!wifiConnected && g_prevWifiConnected && !alarmHigh) {
    g_ledAnim.mode          = LED_ANIM_WIFI_DISCONNECTED;
    g_ledAnim.startMs       = now;
    g_ledAnimDurationMs     = 5000UL;   // 5 seconds
    g_ledAnimBlink          = true;     // blink
    g_ledAnimBlinkStepMs    = 500;      // 500ms on/off for disconnect
    g_ledAnim.active        = true;
  }

  // Render current animation frame based on active mode
  if (g_ledAnim.active) {
    unsigned long elapsed = now - g_ledAnim.startMs;
    switch (g_ledAnim.mode) {
      case LED_ANIM_FIRE_ALERT:
        // Red blinking for 10 seconds at 500 ms on/off
        renderFastBlink(now, 255, 0, 0, 500);
        if (elapsed > 10000UL) {
          g_ledAnim.active = false;
          fillStripColor(0, 0, 0);
        }
        break;
      case LED_ANIM_WIFI_DISCONNECTED:
        // Wi-Fi not connected: either solid or blinking blue based on g_ledAnimBlink
        if (g_ledAnimBlink) {
          // Blink blue (used for mid-session disconnect)
          unsigned long stepMs = (g_ledAnimBlinkStepMs > 0) ? g_ledAnimBlinkStepMs : 200;
          renderFastBlink(now, 0, 0, 80, stepMs);
        } else {
          // Solid blue (used for startup when Wi-Fi is not connected)
          fillStripColor(0, 0, 80);
        }
        if (elapsed > g_ledAnimDurationMs) {
          g_ledAnim.active = false;
          fillStripColor(0, 0, 0);
        }
        break;
      case LED_ANIM_WIFI_CONNECTED_OK:
        // Light blue on/off pattern when Wi-Fi connects (duration set by g_ledAnimDurationMs)
        renderFastBlink(now, 0x2E, 0xFF, 0xB9, g_ledAnimBlinkStepMs > 0 ? g_ledAnimBlinkStepMs : 500);
        if (elapsed > g_ledAnimDurationMs) {
          g_ledAnim.active = false;
          fillStripColor(0, 0, 0);
        }
        break;
      default:
        break;
    }
  } else {
    // No active animation: idle LEDs off
    fillStripColor(0, 0, 0);
  }

  g_prevWifiConnected = wifiConnected;
  g_prevAlarmHigh     = alarmHigh;
g_prevBlynkState    = currState;
#endif
}

// Exposed to BlynkEdgent app_loop() so LED animations run during config/connect loops
void ledAnimationLoop() {
  updateLedAnimation();
}

// ===== Edge Impulse fire alarm inference implementation (from ei_main.cpp) =====

static void firealarm_inference_init()
{
    ei_printf("Edge Impulse FireAlarm inference init (Edgent)\n");

    // Configure buzzer pin
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    // summary of inferencing settings (from model_metadata.h)
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: ");
    ei_printf_float((float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf(" ms.\n");
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / EI_CLASSIFIER_FREQUENCY * 1000);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));

    run_classifier_init();

    if (microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE) == false) {
        ei_printf("ERR: Could not allocate audio buffer (size %d)\r\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
        return;
    }

    ei_printf("Recording (Edgent)...\n");
}

static void firealarm_inference_step()
{
    bool m = microphone_inference_record();
    if (!m) {
        ei_printf("ERR: Failed to record audio...\n");
        return;
    }

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
    signal.get_data = &microphone_audio_signal_get_data;
    ei_impulse_result_t result = {0};

    EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, debug_nn);
    if (r != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", r);
        return;
    }

    // We treat each printed prediction block as one "interval" for alarm logic.
    if (++print_results >= (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)) {
        print_results = 0;

        // --- Extract probabilities for Noise / Alarm and store for external use ---
        float noise_prob = 0.0f;
        float alarm_prob = 0.0f;
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            const char *label = result.classification[ix].label;
            float v = result.classification[ix].value;
            if (!label) continue;
            if (strncmp(label, "Noise", 5) == 0) {
                if (v > noise_prob) noise_prob = v;
            }
            if (strncmp(label, "Alarm", 5) == 0) {
                if (v > alarm_prob) alarm_prob = v;
            }
        }
        g_noise_prob_ml = noise_prob;
        g_alarm_prob_ml = alarm_prob;

        // --- Buzzer control based on Alarm* classes ---
        bool should_start_alarm = false;
        if (alarm_prob >= ALARM_THRESHOLD && alarm_start_ms == 0) {
            should_start_alarm = true;
        }

        unsigned long now_ms = millis();

        if (should_start_alarm) {
            // Start a new 5s buzzer burst
            alarm_start_ms = now_ms;
        }

        // During an active burst, keep fast beeping for 5 seconds regardless of
        // what the model predicts in subsequent intervals.
        if (alarm_start_ms != 0) {
            if (now_ms - alarm_start_ms <= 5000UL) {
                static bool buzzer_state = false;
                buzzer_state = !buzzer_state;
                digitalWrite(BUZZER_PIN, buzzer_state ? HIGH : LOW);
            } else {
                // Burst finished
                alarm_start_ms = 0;
                digitalWrite(BUZZER_PIN, LOW);
            }
        } else {
            // No active burst
            digitalWrite(BUZZER_PIN, LOW);
        }

        // (Optional) Print the predictions to Serial for debugging
        ei_printf("Predictions ");
        ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
            result.timing.dsp, result.timing.classification, result.timing.anomaly);
        ei_printf(": \n");
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            ei_printf("    %s: ", result.classification[ix].label);
            ei_printf_float(result.classification[ix].value);
            ei_printf("\n");
        }
    #if EI_CLASSIFIER_HAS_ANOMALY == 1
        ei_printf("    anomaly score: ");
        ei_printf_float(result.anomaly);
        ei_printf("\n");
    #endif
    }
}

static float firealarm_get_noise_prob() {
    return g_noise_prob_ml;
}

static float firealarm_get_alarm_prob() {
    return g_alarm_prob_ml;
}

// ===== Internal helpers copied from ei_main.cpp =====

static void audio_inference_callback(uint32_t n_bytes)
{
    for (uint32_t i = 0; i < (n_bytes >> 1); i++) {
        inference.buffers[inference.buf_select][inference.buf_count++] = sampleBuffer[i];

        if (inference.buf_count >= inference.n_samples) {
            inference.buf_select ^= 1;
            inference.buf_count = 0;
            inference.buf_ready = 1;
        }
    }
}

static void capture_samples(void* arg) {

  const int32_t i2s_bytes_to_read = (uint32_t)arg;
  size_t bytes_read = i2s_bytes_to_read;

  while (record_status) {

    /* read data at once from i2s */
    i2s_read((i2s_port_t)0, (void*)sampleBuffer, i2s_bytes_to_read, &bytes_read, 100);

    if (bytes_read <= 0) {
      ei_printf("Error in I2S read : %d", bytes_read);
    }
    else {
        if (bytes_read < i2s_bytes_to_read) {
        ei_printf("Partial I2S read");
        }

        // scale the data (otherwise the sound is too quiet)
        for (int x = 0; x < i2s_bytes_to_read/2; x++) {
            sampleBuffer[x] = (int16_t)(sampleBuffer[x]) * 8;
        }

        if (record_status) {
            audio_inference_callback(i2s_bytes_to_read);
        }
        else {
            break;
        }
    }
  }
  vTaskDelete(NULL);
}

static bool microphone_inference_start(uint32_t n_samples)
{
    inference.buffers[0] = (signed short *)malloc(n_samples * sizeof(signed short));

    if (inference.buffers[0] == NULL) {
        return false;
    }

    inference.buffers[1] = (signed short *)malloc(n_samples * sizeof(signed short));

    if (inference.buffers[1] == NULL) {
        ei_free(inference.buffers[0]);
        return false;
    }

    inference.buf_select = 0;
    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;

    if (i2s_init(EI_CLASSIFIER_FREQUENCY)) {
        ei_printf("Failed to start I2S!\n");
    }

    ei_sleep(100);

    record_status = true;

    xTaskCreate(capture_samples, "CaptureSamples", 1024 * 32, (void*)sample_buffer_size, 10, NULL);

    return true;
}

static bool microphone_inference_record(void)
{
    // If the consumer is slower than the producer, we may see an overrun.
    // Instead of treating this as a fatal error, log it once and drop the
    // stale buffer so inference can continue.
    if (inference.buf_ready == 1) {
        ei_printf(
            "Warning: sample buffer overrun. Consider reducing EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW or load.\n");
        inference.buf_ready = 0; // drop oldest buffer
    }

    while (inference.buf_ready == 0) {
        delay(1);
    }

    inference.buf_ready = 0;
    return true;
}

static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr)
{
    numpy::int16_to_float(&inference.buffers[inference.buf_select ^ 1][offset], out_ptr, length);
    return 0;
}

static void microphone_inference_end(void)
{
    i2s_deinit();
    ei_free(inference.buffers[0]);
    ei_free(inference.buffers[1]);
}

// I2S initialization adapted for ESP32-S3 Super Mini + INMP441
// Uses the same pins as your other code: BCLK=GPIO4, WS=GPIO1, SD=GPIO2
static int i2s_init(uint32_t sampling_rate) {
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = sampling_rate,
      .bits_per_sample = (i2s_bits_per_sample_t)16,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_I2S,
      .intr_alloc_flags = 0,
      .dma_buf_count = 8,
      .dma_buf_len = 512,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = -1,
  };
  i2s_pin_config_t pin_config = {
      .bck_io_num = 4,    // BCLK
      .ws_io_num = 1,     // LRCLK / WS
      .data_out_num = -1, // not used
      .data_in_num = 2,   // SD / DOUT
  };
  esp_err_t ret = ESP_OK;

  ret = i2s_driver_install((i2s_port_t)0, &i2s_config, 0, NULL);
  if (ret != ESP_OK) {
    ei_printf("Error in i2s_driver_install\n");
  }

  ret = i2s_set_pin((i2s_port_t)0, &pin_config);
  if (ret != ESP_OK) {
    ei_printf("Error in i2s_set_pin\n");
  }

  ret = i2s_zero_dma_buffer((i2s_port_t)0);
  if (ret != ESP_OK) {
    ei_printf("Error in initializing dma buffer with 0\n");
  }

  return (int)ret;
}

static int i2s_deinit(void) {
    i2s_driver_uninstall((i2s_port_t)0); // stop & destroy i2s driver
    return 0;
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif

