#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

// ===== OLED =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== NEOPIXEL =====
#define LED_PIN 10
#define LED_COUNT 57
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ===== BUTTON =====
#define BUTTON_PIN 9

// ===== MENU =====
int menuIndex = 0;
const int totalModes = 4;
bool inMenu = true;

// ===== BUTTON STATE =====
bool buttonHeld = false;
unsigned long pressStart = 0;

// ===== MODE NAMES =====
const char* modeNames[] = {
  "Rainbow Flow",
  "Color Wipe",
  "Breathing",
  "Sparkle"
};

// ===== SETUP =====
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(7, 6);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

  strip.begin();
  strip.setBrightness((uint8_t)(255 * 0.7));  // 70% brightness
  strip.clear();
  strip.show();

  drawMenu();
}

// ===== LOOP =====
void loop() {
  handleButton();

  if (!inMenu) {
    runEffect();
  }
}

// ===== BUTTON HANDLER =====
void handleButton() {
  bool state = digitalRead(BUTTON_PIN);

  if (state == LOW && !buttonHeld) {
    buttonHeld = true;
    pressStart = millis();
  }

  if (state == HIGH && buttonHeld) {
    unsigned long pressTime = millis() - pressStart;
    buttonHeld = false;

    if (pressTime < 800) {
      // SHORT PRESS
      if (inMenu) {
        menuIndex = (menuIndex + 1) % totalModes;
        drawMenu();
      } else {
        inMenu = true;   // stop effect and return to menu
        strip.clear();
        strip.show();
        drawMenu();
      }
    } else {
      // LONG PRESS
      if (inMenu) {
        inMenu = false;
        strip.clear();
        strip.show();
      }
    }
  }
}

// ===== OLED MENU =====
void drawMenu() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Select Light Mode:");

  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setTextSize(1);

  for (int i = 0; i < totalModes; i++) {
    if (i == menuIndex) {
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // highlighted
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(0, 20 + i * 12);
    display.println(modeNames[i]);
  }

  display.setTextColor(SSD1306_WHITE);
  display.display();
}

// ===== RUN EFFECT =====
void runEffect() {
  switch (menuIndex) {
    case 0: rainbowFlow(); break;
    case 1: colorWipe(); break;
    case 2: breathing(); break;
    case 3: sparkle(); break;
  }
}

// ===== EFFECT 1: RAINBOW =====
void rainbowFlow() {
  static uint16_t hue = 0;
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i,
      strip.gamma32(strip.ColorHSV(hue + i * 400)));
  }
  strip.show();
  hue += 200;
  delay(30);
}

// ===== EFFECT 2: COLOR WIPE =====
void colorWipe() {
  static int index = 0;

  strip.setPixelColor(index, 255, 0, 0);
  strip.show();
  index++;

  if (index >= LED_COUNT) {
    index = 0;
    strip.clear();
  }
  delay(40);
}

// ===== EFFECT 3: BREATHING =====
void breathing() {
  static int brightness = 179; // 70% of 255
  static int step = 4;

  brightness += step;
  if (brightness <= 50 || brightness >= 179) step = -step; // keep 20%-70%

  strip.setBrightness(brightness);
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, 0, 0, 255);
  }
  strip.show();
  delay(20);
}

// ===== EFFECT 4: SPARKLE =====
void sparkle() {
  strip.clear();
  for (int i = 0; i < 6; i++) {
    int p = random(LED_COUNT);
    strip.setPixelColor(p, 255, 255, 255);
  }
  strip.show();
  delay(80);
}
