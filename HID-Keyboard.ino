/*
  ESP32-S2 Lolin S2 Mini
  Keyboard Matrix → USB HID
  Using TinyUSB built-in keyboard
*/

#include "USB.h"
#include "USBHIDKeyboard.h"

// ---------- Matrix size ----------
const uint8_t ROWS = 4;
const uint8_t COLS = 5;

// ---------- Pins (adjust to your wiring) ----------
// Pick GPIOs safe for matrix scanning
const uint8_t rowPins[ROWS] = {2, 3, 4, 5};    // example, check your board pinout
const uint8_t colPins[COLS] = {6, 7, 8, 9, 10};

// ---------- Debounce ----------
const uint16_t DEBOUNCE_MS = 10;

// ---------- HID Keyboard ----------
USBHIDKeyboard keyboard;
USBHID hid;

// ---------- Key map ----------
// Use HID_KEY_* from TinyUSB
const uint8_t keymap[ROWS][COLS] = {
  { HID_KEY_Q, HID_KEY_W, HID_KEY_E, HID_KEY_R, HID_KEY_T },
  { HID_KEY_A, HID_KEY_S, HID_KEY_D, HID_KEY_F, HID_KEY_G },
  { HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_V, HID_KEY_B },
  { HID_KEY_CONTROL_LEFT, HID_KEY_SHIFT_LEFT, HID_KEY_SPACE, HID_KEY_ENTER, HID_KEY_ESCAPE }
};

// ---------- State tracking ----------
bool keyState[ROWS][COLS];
bool keyStatePrev[ROWS][COLS];
uint32_t lastChangeMs[ROWS][COLS];

void setup() {
  // Rows as outputs, idle HIGH
  for (uint8_t r = 0; r < ROWS; r++) {
    pinMode(rowPins[r], OUTPUT);
    digitalWrite(rowPins[r], HIGH);
  }

  // Columns as inputs with pullups
  for (uint8_t c = 0; c < COLS; c++) {
    pinMode(colPins[c], INPUT_PULLUP);
  }

  // Init states
  for (uint8_t r = 0; r < ROWS; r++) {
    for (uint8_t c = 0; c < COLS; c++) {
      keyState[r][c] = false;
      keyStatePrev[r][c] = false;
      lastChangeMs[r][c] = 0;
    }
  }

  // Start USB HID
  USB.begin();
  hid.begin();
  keyboard.begin();
}

inline bool readSwitch(uint8_t r, uint8_t c) {
  return (digitalRead(colPins[c]) == LOW);
}

void scanMatrix() {
  for (uint8_t r = 0; r < ROWS; r++) {
    digitalWrite(rowPins[r], LOW);
    delayMicroseconds(3);

    for (uint8_t c = 0; c < COLS; c++) {
      bool rawPressed = readSwitch(r, c);
      bool current = keyState[r][c];
      uint32_t now = millis();

      if (rawPressed != current) {
        if ((now - lastChangeMs[r][c]) >= DEBOUNCE_MS) {
          keyState[r][c] = rawPressed;
          lastChangeMs[r][c] = now;
        }
      } else {
        lastChangeMs[r][c] = now;
      }
    }

    digitalWrite(rowPins[r], HIGH);
  }
}

void sendHIDChanges() {
  for (uint8_t r = 0; r < ROWS; r++) {
    for (uint8_t c = 0; c < COLS; c++) {
      bool now = keyState[r][c];
      bool was = keyStatePrev[r][c];
      if (now != was) {
        uint8_t kc = keymap[r][c];
        if (kc != 0) {
          if (now) {
            keyboard.press(kc);
          } else {
            keyboard.release(kc);
          }
        }
        keyStatePrev[r][c] = now;
      }
    }
  }
}

void loop() {
  scanMatrix();
  sendHIDChanges();
  delay(1);
}
