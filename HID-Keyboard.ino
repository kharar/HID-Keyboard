/*
  Pro Micro Keyboard Matrix → USB HID
  Uses NicoHood HID-Project (BootKeyboard) for clean HID keycodes.

  Wiring (example 4x5):
  Rows:    2,3,4,5       (OUTPUT, set LOW one-at-a-time)
  Columns: 6,7,8,9,10    (INPUT_PULLUP)
*/

#include <HID-Project.h>
#include <HID-Settings.h>

// ---------- Matrix size ----------
const uint8_t ROWS = 4;
const uint8_t COLS = 5;

// ---------- Pins (adjust to your wiring) ----------
const uint8_t rowPins[ROWS] = {2, 3, 4, 5};
const uint8_t colPins[COLS] = {6, 7, 8, 9, 10};

// ---------- Debounce ----------
const uint16_t DEBOUNCE_MS = 10;

// ---------- Key map ----------
// Use KEY_* from HID-Project. Use 0 for "no key".
const KeyboardKeycode keymap[ROWS][COLS] = {
  // c0          c1             c2        c3         c4
  { KEY_Q,        KEY_W,        KEY_E,    KEY_R,     KEY_T    },  // r0
  { KEY_A,        KEY_S,        KEY_D,    KEY_F,     KEY_G    },  // r1
  { KEY_Z,        KEY_X,        KEY_C,    KEY_V,     KEY_B    },  // r2
  { KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_SPACE, KEY_ENTER, KEY_ESC }   // r3
};

// ---------- State tracking ----------
bool keyState[ROWS][COLS];          // stable debounced state
bool keyStatePrev[ROWS][COLS];      // previous stable state
uint32_t lastChangeMs[ROWS][COLS];  // last time we saw a transition (for debounce)

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

  // Clear states
  for (uint8_t r = 0; r < ROWS; r++) {
    for (uint8_t c = 0; c < COLS; c++) {
      keyState[r][c] = false;
      keyStatePrev[r][c] = false;
      lastChangeMs[r][c] = 0;
    }
  }

  BootKeyboard.begin(); // Start USB HID
  delay(50);
}

// Read one key (r,c) with current row driven LOW
inline bool readSwitch(uint8_t r, uint8_t c) {
  // With row LOW and pullup on column, pressed = LOW
  return (digitalRead(colPins[c]) == LOW);
}

void scanMatrix() {
  for (uint8_t r = 0; r < ROWS; r++) {
    // Drive current row LOW
    digitalWrite(rowPins[r], LOW);
    delayMicroseconds(3); // settle a tiny bit

    for (uint8_t c = 0; c < COLS; c++) {
      bool rawPressed = readSwitch(r, c);
      bool current = keyState[r][c];
      uint32_t now = millis();

      // Debounce logic
      if (rawPressed != current) {
        if ((now - lastChangeMs[r][c]) >= DEBOUNCE_MS) {
          keyState[r][c] = rawPressed;
          lastChangeMs[r][c] = now;
        }
      } else {
        lastChangeMs[r][c] = now;
      }
    }

    // Release row (idle HIGH)
    digitalWrite(rowPins[r], HIGH);
  }
}

void sendHIDChanges() {
  for (uint8_t r = 0; r < ROWS; r++) {
    for (uint8_t c = 0; c < COLS; c++) {
      bool now = keyState[r][c];
      bool was = keyStatePrev[r][c];
      if (now != was) {
        KeyboardKeycode kc = keymap[r][c];
        if (kc != 0) {   // 0 = no key assigned
          if (now) {
            BootKeyboard.press(kc);
          } else {
            BootKeyboard.release(kc);
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
  delay(1); // small idle time
}
