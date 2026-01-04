// // // #include <Wire.h>
// // // #include "HUSKYLENS.h"

// // // HUSKYLENS huskylens;

// // // // user settings
// // // const unsigned long SHOT_INTERVAL_MS = 3000; // must keep >= 500 ms between saves
// // // bool saveScreenshot = false;                 // false = clean photo, true = screenshot with UI

// // // void setup() {
// // //   Serial.begin(9600);
// // //   Wire.begin(); // Nano I2C: A4(SDA), A5(SCL)

// // //   Serial.println(F("[Husky] Connecting over I2C..."));
// // //   while (!huskylens.begin(Wire)) {
// // //     Serial.println(F("[Husky] Begin failed. Check: Protocol=I2C, A4->T(SDA), A5->R(SCL), power."));
// // //     delay(300);
// // //   }
// // //   Serial.println(F("[Husky] Connected. Saving an image every 3 s..."));
// // //   delay(500); // small settle helps SD init after power-up
// // // }

// // // void loop() {
// // //   static unsigned long last = 0;
// // //   static uint32_t count = 0;
// // //   unsigned long now = millis();

// // //   if (now - last >= SHOT_INTERVAL_MS) {
// // //     last = now;

// // //     bool ok = saveScreenshot
// // //               ? huskylens.saveScreenshotToSDCard()
// // //               : huskylens.savePictureToSDCard();

// // //     if (ok) {
// // //       Serial.print(F("Saved image #"));
// // //       Serial.println(++count);
// // //     } else {
// // //       Serial.println(F("Save failed (check: microSD FAT32/MBR, fully seated, on live camera view)."));
// // //     }

// // //     // Ensure >0.5 s gap between saves (device requirement)
// // //     delay(600);
// // //   }
// // // }


// // #include <Wire.h>
// // #include "HUSKYLENS.h"

// // HUSKYLENS huskylens;

// // // === user settings ===
// // const unsigned long SHOT_INTERVAL_MS = 3000;   // overall cadence
// // const bool SAVE_SCREENSHOT = false;            // false = clean photo, true = screenshot (with UI)

// // static bool saveImageWithRetry() {
// //   // try up to 3 times to account for late acks while the camera is busy
// //   for (uint8_t tries = 0; tries < 3; tries++) {
// //     bool ok = SAVE_SCREENSHOT ? huskylens.saveScreenshotToSDCard()
// //                               : huskylens.savePictureToSDCard();
// //     if (ok) return true;
// //     delay(250);  // small backoff; HuskyLens may still be finalizing the previous write
// //   }
// //   return false;
// // }

// // void setup() {
// //   Serial.begin(9600);

// //   Wire.begin();                    // A4 = SDA (T), A5 = SCL (R)
// //   Wire.setClock(100000);           // force 100kHz (most stable)
// // #if defined(WIRE_HAS_TIMEOUT)
// //   Wire.setWireTimeout(3000, true); // wait up to 3s for I2C before bailing; reset on timeout
// // #endif

// //   Serial.println(F("[Husky] Connecting over I2C..."));
// //   while (!huskylens.begin(Wire)) {
// //     Serial.println(F("[Husky] Begin failed. Check: Protocol=I2C, A4->T(SDA), A5->R(SCL), power."));
// //     delay(300);
// //   }
// //   Serial.println(F("[Husky] Connected. Saving an image every 3 s..."));
// //   delay(600); // settle to ensure SD is fully ready after power-up
// // }

// // void loop() {
// //   static unsigned long last = 0;
// //   if (millis() - last >= SHOT_INTERVAL_MS) {
// //     last = millis();

// //     bool ok = saveImageWithRetry();
// //     if (ok) {
// //       static uint32_t count = 0;
// //       Serial.print(F("Saved image #")); Serial.println(++count);
// //     } else {
// //       Serial.println(F("Save reported FAIL (image may still have saved)."));
// //     }

// //     // extra margin beyond the 0.5 s requirement to avoid late acks
// //     delay(900);
// //   }
// // }

// /*
//   HuskyLens Reliable Capture (Arduino Nano)
//   - Uses I2C by default; switch to UART by setting USE_UART to 1.
//   - Adds robust retries, enforced cool-off, and auto-recovery.
//   - Tiny serial console for control.

//   Serial (USB): 9600 8N1
// */

// #include <Arduino.h>
// #include <Wire.h>
// #include "HUSKYLENS.h"

// //////////////////// USER OPTIONS ////////////////////
// #define USE_UART 0            // 0 = I2C (A4/A5), 1 = UART (D8/D9 on Nano via SoftwareSerial)
// unsigned long SHOT_INTERVAL_MS = 4000;  // safer default cadence
// bool SAVE_SCREENSHOT = false;            // false=clean photo, true=overlay UI
// const uint8_t MAX_TRIES = 3;             // retries per shot
// const uint16_t TRY_BACKOFF_MS = 350;     // wait between retries
// const uint16_t POST_SAVE_COOLDOWN_MS = 1500; // extra margin after a save (prevents false fail)
// const uint16_t RECONNECT_DELAY_MS = 600; // wait before re-trying begin()
// //////////////////////////////////////////////////////

// HUSKYLENS huskylens;

// #if USE_UART
//   #include <SoftwareSerial.h>
//   // Nano: D8 as RX, D9 as TX (adjust if needed)
//   SoftwareSerial HLSerial(8, 9); // RX, TX
// #endif

// // state
// static bool autoCapture = true;
// static uint32_t imageCount = 0;
// static unsigned long lastShotMs = 0;

// static void printHelp() {
//   Serial.println(F("\nCommands:"));
//   Serial.println(F("  p        -> capture once now"));
//   Serial.println(F("  a        -> toggle auto capture on/off"));
//   Serial.println(F("  s        -> toggle screenshot (overlay) on/off"));
//   Serial.println(F("  i####    -> set interval ms (e.g., i3000)"));
//   Serial.println(F("  ?        -> show status\n"));
// }

// static void printStatus() {
//   Serial.println(F("=== HuskyLens Capture Status ==="));
//   Serial.print(F("Interface: ")); Serial.println(USE_UART ? F("UART") : F("I2C"));
//   Serial.print(F("Auto: ")); Serial.println(autoCapture ? F("ON") : F("OFF"));
//   Serial.print(F("Screenshot overlay: ")); Serial.println(SAVE_SCREENSHOT ? F("ON") : F("OFF"));
//   Serial.print(F("Interval (ms): ")); Serial.println(SHOT_INTERVAL_MS);
//   Serial.print(F("Saved count: ")); Serial.println(imageCount);
//   Serial.println(F("================================"));
// }

// static bool ensureConnected() {
//   // attempt to (re)start communication if needed
//   for (uint8_t attempt = 0; attempt < 3; attempt++) {
//     if (huskylens.isLearned() || huskylens.requestAll()) {
//       // quick ping path often true if already alive
//       return true;
//     }
//     Serial.println(F("[Husky] (Re)init link..."));
// #if USE_UART
//     // begin() on UART stream
//     if (!huskylens.begin(HLSerial)) {
//       Serial.println(F("[Husky] begin(UART) failed."));
//     } else {
//       // one small ping to stabilize
//       delay(RECONNECT_DELAY_MS);
//       return true;
//     }
// #else
//     if (!huskylens.begin(Wire)) {
//       Serial.println(F("[Husky] begin(I2C) failed."));
//     } else {
//       delay(RECONNECT_DELAY_MS);
//       return true;
//     }
// #endif
//     delay(RECONNECT_DELAY_MS);
//   }
//   return false;
// }

// static bool saveOnceWithRetries() {
//   bool ok = false;
//   for (uint8_t tries = 0; tries < MAX_TRIES; tries++) {
//     ok = SAVE_SCREENSHOT ? huskylens.saveScreenshotToSDCard()
//                          : huskylens.savePictureToSDCard();
//     if (ok) {
//       return true;
//     }
//     // If the camera is finalizing previous write, give it breath
//     delay(TRY_BACKOFF_MS);
//   }
//   return false;
// }

// static void doCapture() {
//   if (!ensureConnected()) {
//     Serial.println(F("[Husky] Link down. Will retry next cycle."));
//     return;
//   }

//   const bool ok = saveOnceWithRetries();
//   if (ok) {
//     imageCount++;
//     Serial.print(F("Saved image #")); Serial.println(imageCount);
//     // important: enforce extra cool-down so HuskyLens has ample time to finalize writes
//     delay(POST_SAVE_COOLDOWN_MS);
//   } else {
//     // Note: in rare cases, file can still land on SD but API returns false.
//     Serial.println(F("Save reported FAIL (file may still have saved)."));
//     // Add a longer cool-down to let FW recover before next cycle
//     delay(POST_SAVE_COOLDOWN_MS + 400);
//   }
// }

// static void handleConsole() {
//   if (!Serial.available()) return;
//   String cmd = Serial.readStringUntil('\n');
//   cmd.trim();
//   if (cmd.length() == 0) return;

//   switch (cmd.charAt(0)) {
//     case 'p':
//     case 'P':
//       Serial.println(F("[Console] Single capture..."));
//       doCapture();
//       break;
//     case 'a':
//     case 'A':
//       autoCapture = !autoCapture;
//       Serial.print(F("[Console] Auto capture: "));
//       Serial.println(autoCapture ? F("ON") : F("OFF"));
//       break;
//     case 's':
//     case 'S':
//       SAVE_SCREENSHOT = !SAVE_SCREENSHOT;
//       Serial.print(F("[Console] Screenshot overlay: "));
//       Serial.println(SAVE_SCREENSHOT ? F("ON") : F("OFF"));
//       break;
//     case 'i':
//     case 'I': {
//       // format: i#### (ms)
//       unsigned long v = cmd.substring(1).toInt();
//       if (v >= 600) { // enforce sane minimum (Husky FW needs >0.5 s between saves)
//         SHOT_INTERVAL_MS = v;
//         Serial.print(F("[Console] Interval set to ")); Serial.print(SHOT_INTERVAL_MS); Serial.println(F(" ms"));
//       } else {
//         Serial.println(F("[Console] Interval too small; must be >= 600 ms."));
//       }
//       break;
//     }
//     case '?':
//       printStatus();
//       break;
//     default:
//       printHelp();
//       break;
//   }
// }

// void setup() {
//   Serial.begin(9600);
//   while (!Serial) { /* wait for USB on native boards; on Nano this returns immediately */ }

//   Serial.println(F("\n[Husky] Reliable Capture starting..."));

// #if USE_UART
//   HLSerial.begin(9600); // HuskyLens default UART baud
//   delay(50);
// #else
//   Wire.begin();                    // A4 = SDA(T), A5 = SCL(R)
//   Wire.setClock(100000);           // 100 kHz for stability
//   #if defined(WIRE_HAS_TIMEOUT)
//     Wire.setWireTimeout(3000, true); // reset on timeout
//   #endif
//   delay(50);
// #endif

//   if (!ensureConnected()) {
//     Serial.println(F("[Husky] Could not connect yet; will keep trying in loop."));
//   } else {
//     Serial.println(F("[Husky] Connected. Auto capture is ON by default."));
//   }

//   printHelp();
//   printStatus();

//   // Give SD time to settle after power-up
//   delay(600);
// }

// void loop() {
//   handleConsole();

//   const unsigned long now = millis();
//   if (autoCapture && (now - lastShotMs >= SHOT_INTERVAL_MS)) {
//     lastShotMs = now;
//     doCapture();
//   }

//   // small idle delay keeps console snappy without busy looping
//   delay(5);
// }

//_______

/*
  HuskyLens Reliable Capture (Arduino Nano)
  - Default: I2C on A4/A5
  - Optional: UART on D8/D9 (set USE_UART = 1)
  - Robust retries, enforced cool-off, auto-recovery, and console commands

  Serial console (USB): 9600 baud, 8N1
  Commands:
    p        -> capture once now
    a        -> toggle auto capture on/off
    s        -> toggle screenshot overlay on/off
    i####    -> set interval ms (e.g., i5000)
    ?        -> show status
*/

#include <Arduino.h>
#include <Wire.h>
#include "HUSKYLENS.h"

//////////////////// USER OPTIONS ////////////////////
#define USE_UART 0                  // 0 = I2C (A4/A5), 1 = UART (D8/D9 via SoftwareSerial)
unsigned long SHOT_INTERVAL_MS = 5000;   // safer default cadence
bool SAVE_SCREENSHOT = false;            // false = clean photo, true = overlay UI
const uint8_t  MAX_TRIES = 3;            // retries per shot
const uint16_t TRY_BACKOFF_MS = 500;     // wait between retries
const uint16_t POST_SAVE_COOLDOWN_MS = 2000; // enforced gap after save
const uint16_t RECONNECT_DELAY_MS = 600;     // wait before retrying begin()
//////////////////////////////////////////////////////

HUSKYLENS huskylens;

#if USE_UART
  #include <SoftwareSerial.h>
  // Nano: D8 = RX, D9 = TX (adjust if needed)
  SoftwareSerial HLSerial(8, 9); // RX, TX
#endif

// state
static bool autoCapture = true;
static uint32_t imageCount = 0;
static unsigned long lastShotMs = 0;

static void printHelp() {
  Serial.println(F("\nCommands:"));
  Serial.println(F("  p        -> capture once now"));
  Serial.println(F("  a        -> toggle auto capture on/off"));
  Serial.println(F("  s        -> toggle screenshot overlay on/off"));
  Serial.println(F("  i####    -> set interval ms (e.g., i5000)"));
  Serial.println(F("  ?        -> show status\n"));
}

static void printStatus() {
  Serial.println(F("=== HuskyLens Capture Status ==="));
  Serial.print(F("Interface: ")); Serial.println(USE_UART ? F("UART") : F("I2C"));
  Serial.print(F("Auto: ")); Serial.println(autoCapture ? F("ON") : F("OFF"));
  Serial.print(F("Screenshot overlay: ")); Serial.println(SAVE_SCREENSHOT ? F("ON") : F("OFF"));
  Serial.print(F("Interval (ms): ")); Serial.println(SHOT_INTERVAL_MS);
  Serial.print(F("Saved count: ")); Serial.println(imageCount);
  Serial.println(F("================================"));
}

// Returns true if the device is responsive.
// Uses request() as a lightweight ping; no requestAll() in this library.
static bool ensureConnected() {
  for (uint8_t attempt = 0; attempt < 3; attempt++) {
    // If already begun and responsive, request() will succeed.
    if (huskylens.request()) return true;

    Serial.println(F("[Husky] (Re)init link..."));
#if USE_UART
    if (!huskylens.begin(HLSerial)) {
      Serial.println(F("[Husky] begin(UART) failed."));
    } else {
      delay(RECONNECT_DELAY_MS);
      if (huskylens.request()) return true;
    }
#else
    if (!huskylens.begin(Wire)) {
      Serial.println(F("[Husky] begin(I2C) failed."));
    } else {
      delay(RECONNECT_DELAY_MS);
      if (huskylens.request()) return true;
    }
#endif
    delay(RECONNECT_DELAY_MS);
  }
  return false;
}

static bool saveOnceWithRetries() {
  for (uint8_t tries = 0; tries < MAX_TRIES; tries++) {
    bool ok = SAVE_SCREENSHOT ? huskylens.saveScreenshotToSDCard()
                              : huskylens.savePictureToSDCard();
    if (ok) return true;
    delay(TRY_BACKOFF_MS);  // camera may still be finalizing last write
  }
  return false;
}

static void doCapture() {
  if (!ensureConnected()) {
    Serial.println(F("[Husky] Link down. Will retry next cycle."));
    return;
  }

  const bool ok = saveOnceWithRetries();
  if (ok) {
    imageCount++;
    Serial.print(F("Saved image #")); Serial.println(imageCount);
    delay(POST_SAVE_COOLDOWN_MS); // enforce safe cool-down
  } else {
    Serial.println(F("Save reported FAIL (file may still exist)."));
    delay(POST_SAVE_COOLDOWN_MS + 400); // longer rest if fail
  }
}

static void handleConsole() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.length() == 0) return;

  switch (cmd.charAt(0)) {
    case 'p': case 'P':
      Serial.println(F("[Console] Single capture..."));
      doCapture();
      break;
    case 'a': case 'A':
      autoCapture = !autoCapture;
      Serial.print(F("[Console] Auto capture: "));
      Serial.println(autoCapture ? F("ON") : F("OFF"));
      break;
    case 's': case 'S':
      SAVE_SCREENSHOT = !SAVE_SCREENSHOT;
      Serial.print(F("[Console] Screenshot overlay: "));
      Serial.println(SAVE_SCREENSHOT ? F("ON") : F("OFF"));
      break;
    case 'i': case 'I': {
      unsigned long v = cmd.substring(1).toInt();
      if (v >= 600) {
        SHOT_INTERVAL_MS = v;
        Serial.print(F("[Console] Interval set to "));
        Serial.print(SHOT_INTERVAL_MS); Serial.println(F(" ms"));
      } else {
        Serial.println(F("[Console] Interval too small; must be >= 600 ms."));
      }
      break;
    }
    case '?':
      printStatus();
      break;
    default:
      printHelp();
      break;
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial) { /* wait for USB on native boards; Nano returns immediately */ }

  Serial.println(F("\n[Husky] Reliable Capture starting..."));

#if USE_UART
  HLSerial.begin(9600); // HuskyLens default UART baud
  delay(50);
#else
  Wire.begin();                    // A4 = SDA(T), A5 = SCL(R)
  Wire.setClock(100000);           // 100 kHz for stability
  #if defined(WIRE_HAS_TIMEOUT)
    Wire.setWireTimeout(3000, true); // reset on timeout
  #endif
  delay(50);
#endif

  if (!ensureConnected()) {
    Serial.println(F("[Husky] Could not connect yet; will retry in loop."));
  } else {
    Serial.println(F("[Husky] Connected. Auto capture is ON (every 5 s)."));
  }

  printHelp();
  printStatus();

  delay(600); // give SD time to settle after power-up
}

void loop() {
  handleConsole();

  const unsigned long now = millis();
  if (autoCapture && (now - lastShotMs >= SHOT_INTERVAL_MS)) {
    lastShotMs = now;
    doCapture();
  }

  delay(5); // idle delay keeps console responsive
}

