#define ARDUINO_USB_CDC_ON_BOOT 1

#include <Arduino.h>
#include "driver/i2s.h"

// =============================
// ESP32-S3 Super Mini + I2S MEMS mic
// =============================
// Wire your MEMS mic like this (3.3 V mic, I2S interface):
//   Mic 3V3  -> 3V3 on S3 Super Mini
//   Mic GND  -> GND on S3 Super Mini
//   Mic SCK / BCLK -> IO4  (GPIO4)
//   Mic WS  / LRCLK -> IO1 (GPIO1)
//   Mic SD  / DOUT  -> IO2 (GPIO2)
// NOTE: These are **not** I2C SDA/SCL pins. I2S uses SCK, WS and SD.

// I2S pin configuration for ESP32-S3 Super Mini
#define I2S_WS   1   // GPIO1  - word select (LRCLK / WS)
#define I2S_SD   2   // GPIO2  - data in (SD / DOUT)
#define I2S_SCK  4   // GPIO4  - bit clock (BCLK / SCK)

#define SAMPLE_RATE   16000
#define I2S_READ_LEN  1024

// NOTE: Many MEMS mics (e.g. INMP441) output on the LEFT channel when L/R pin is tied to GND.
// So we configure I2S to read the LEFT channel.
i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
};

i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
};

void setup() {
    Serial.begin(115200);
    delay(2000); // wait for Serial USB to connect
    Serial.println("MEMS microphone ready (ESP32-S3 Super Mini)");

    // Install and start I2S driver
    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.print("i2s_driver_install error: ");
        Serial.println(err);
    }

    err = i2s_set_pin(I2S_NUM_0, &pin_config);
    if (err != ESP_OK) {
        Serial.print("i2s_set_pin error: ");
        Serial.println(err);
    }
}

void loop() {
    int32_t buffer[I2S_READ_LEN];
    size_t bytes_read = 0;

    // Read samples from MEMS microphone
    esp_err_t err = i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);
    if (err != ESP_OK) {
        Serial.print("i2s_read error: ");
        Serial.println(err);
        delay(500);
        return;
    }

    // Convert raw 32-bit samples to a simple "level" value.
    // Most I2S mics put useful data in the high bits, so we right-shift.
    int sample_count = bytes_read / sizeof(int32_t);
    long long sum_abs = 0;
    for (int i = 0; i < sample_count; i++) {
        int32_t s = buffer[i] >> 14;   // shrink to ~18-bit / 16-bit range
        if (s < 0) s = -s;
        sum_abs += s;
    }
    int32_t level = (int32_t)(sum_abs / sample_count);

    // Map level to a simple ASCII bar for serial output
    // Adjust SCALE to change sensitivity (smaller = more sensitive)
    const int MAX_BAR = 40;
    const int SCALE = 200; // tune this when mic is connected
    int barLen = level / SCALE;
    if (barLen > MAX_BAR) barLen = MAX_BAR;
    if (barLen < 0) barLen = 0;

    Serial.print("Level:");
    Serial.print(level);
    Serial.print(" ");
    Serial.print("|");
    for (int i = 0; i < barLen; i++) {
        Serial.print("#");
    }
    Serial.println("|");

    delay(100);
}
