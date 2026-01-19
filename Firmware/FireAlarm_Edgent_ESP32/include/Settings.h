
/*
 * Board configuration (see examples below).
 */

#if defined(USE_WROVER_BOARD)

  #define BOARD_BUTTON_PIN            15
  #define BOARD_BUTTON_ACTIVE_LOW     true

  #define BOARD_LED_PIN_R             0
  #define BOARD_LED_PIN_G             2
  #define BOARD_LED_PIN_B             4
  #define BOARD_LED_INVERSE           false
  #define BOARD_LED_BRIGHTNESS        128

#elif defined(USE_TTGO_T7)

  #warning "This board does not have a button. Connect a button to gpio0 <> GND"

  #define BOARD_BUTTON_PIN            0
  #define BOARD_BUTTON_ACTIVE_LOW     true

  #define BOARD_LED_PIN               19
  #define BOARD_LED_INVERSE           false
  #define BOARD_LED_BRIGHTNESS        64

#elif defined(USE_TTGO_T7_S3)

  #define BOARD_BUTTON_PIN            0
  #define BOARD_BUTTON_ACTIVE_LOW     true

  #define BOARD_LED_PIN               17
  #define BOARD_LED_INVERSE           false
  #define BOARD_LED_BRIGHTNESS        64

#elif defined(USE_TTGO_T_OI)

  #warning "This board does not have a button. Connect a button to gpio0 <> GND"

  #define BOARD_BUTTON_PIN            0
  #define BOARD_BUTTON_ACTIVE_LOW     true

  #define BOARD_LED_PIN               3
  #define BOARD_LED_INVERSE           false
  #define BOARD_LED_BRIGHTNESS        64

#elif defined(USE_ESP32_DEV_MODULE)

  #warning "The LED of this board is not configured"

  #define BOARD_BUTTON_PIN            0
  #define BOARD_BUTTON_ACTIVE_LOW     true

#elif defined(USE_ESP32C3_DEV_MODULE)

  #define BOARD_BUTTON_PIN            9
  #define BOARD_BUTTON_ACTIVE_LOW     true

  #define BOARD_LED_PIN_WS2812        8
  #define BOARD_LED_INVERSE           false
  #define BOARD_LED_BRIGHTNESS        32

#elif defined(USE_ESP32S2_DEV_KIT)

  #define BOARD_BUTTON_PIN            0
  #define BOARD_BUTTON_ACTIVE_LOW     true

  #define BOARD_LED_PIN               19
  #define BOARD_LED_INVERSE           false
  #define BOARD_LED_BRIGHTNESS        128

#else
 
  #warning "Custom board configuration is used"
 
  // ESP32-S3 Super Mini custom config
  // User button on GPIO10, active-LOW (button to GND, internal pull-up enabled)
  #define BOARD_BUTTON_PIN            9                    // Pin where user button is attached
  #define BOARD_BUTTON_ACTIVE_LOW     true                  // true if button is "active-low"
 
  // All visual indication uses external WS2812 strip on GPIO11
  // Built-in LEDs and any BOARD_LED_PIN / BOARD_LED_PIN_R/G/B are intentionally not used
  #define BOARD_LED_PIN_WS2812        10                    // WS2812 / NeoPixel strip data pin
  #define BOARD_NEOPIXEL_COUNT        57                    // Number of LEDs in the strip
  #define BOARD_LED_INVERSE           false                 // WS2812 is not inverted
  #define BOARD_LED_BRIGHTNESS        100                    // ~20% of 255 to limit current
 
#endif


/*
 * Advanced options
 */

#define BUTTON_HOLD_TIME_INDICATION   3000
#define BUTTON_HOLD_TIME_ACTION       10000
#define BUTTON_PRESS_TIME_ACTION      50

#define BOARD_PWM_MAX                 1023

#if !defined(CONFIG_DEVICE_PREFIX)
#define CONFIG_DEVICE_PREFIX          "FireAlarm Detector"
#endif
#if !defined(CONFIG_AP_URL)
#define CONFIG_AP_URL                 "blynk.setup"
#endif
#if !defined(CONFIG_DEFAULT_SERVER)
#define CONFIG_DEFAULT_SERVER         "blynk.cloud"
#endif
#if !defined(CONFIG_DEFAULT_PORT)
#define CONFIG_DEFAULT_PORT           443
#endif

#define WIFI_CLOUD_MAX_RETRIES        500
#define WIFI_NET_CONNECT_TIMEOUT      50000
#define WIFI_CLOUD_CONNECT_TIMEOUT    50000
#define WIFI_AP_IP                    IPAddress(192, 168, 4, 1)
#define WIFI_AP_Subnet                IPAddress(255, 255, 255, 0)
//#define WIFI_CAPTIVE_PORTAL_ENABLE

//#define USE_TICKER
//#define USE_TIMER_ONE
//#define USE_TIMER_THREE
//#define USE_TIMER_FIVE
#define USE_PTHREAD

// Disable built-in analog and digital pin control
#define BLYNK_NO_BUILTIN
#define BLYNK_NO_DEFAULT_BANNER

#if defined(APP_DEBUG)
  #define DEBUG_PRINT(...)  BLYNK_LOG1(__VA_ARGS__)
  #define DEBUG_PRINTF(...) BLYNK_LOG(__VA_ARGS__)
#else
  #define DEBUG_PRINT(...)
  #define DEBUG_PRINTF(...)
#endif

