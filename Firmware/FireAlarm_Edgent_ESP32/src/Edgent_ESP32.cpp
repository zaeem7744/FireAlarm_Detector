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

  // Initialize Edge Impulse-based fire alarm inference
  firealarm_inference_init();

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

    // We intentionally do not print IP to keep the display clean
  } else {
    display.println("DISCONNECTED");
  }

  // Show LED strip logical state from Blynk
  display.setCursor(0, 48);
  display.print("LED: ");
  display.println(g_ledOn ? "ON" : "OFF");

  // Show ML confidences (Noise / Alarm) on the next line with full labels
  float noiseProb = firealarm_get_noise_prob();
  float alarmProb = firealarm_get_alarm_prob();
  display.setCursor(0, 56);
  display.print("Noise: ");
  display.print(noiseProb, 2);
  display.print("  Alarm: ");
  display.print(alarmProb, 2);

  display.display();
}
 
void loop() {
  BlynkEdgent.run();
  // Run one step of the Edge Impulse fire alarm inference inside Edgent loop
  firealarm_inference_step();
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

