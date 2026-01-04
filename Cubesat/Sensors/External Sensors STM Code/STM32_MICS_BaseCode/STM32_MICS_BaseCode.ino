//Connect MICS to specified pins 5V AND Gnd
// CODE1: TO CHECK PINS WORKING AND VOLTAGE

// #include <Arduino.h>

// HardwareSerial Serial1(PA10, PA9);   // Your BT/UART port

// // --- Pin map from your header image ---
// #define MICS_EN   PC6   // enable pin (drive HIGH to turn sensor on)
// #define MICS_AO   PB0   // analog input (ADC)

// // Optional: flip this if your EN is active-LOW on your breakout
// const bool EN_ACTIVE_HIGH = true;

// void setup() {
//   Serial1.begin(115200, SERIAL_8E1);

//   pinMode(MICS_EN, OUTPUT);
//   digitalWrite(MICS_EN, EN_ACTIVE_HIGH ? HIGH : LOW);  // turn sensor on

//   analogReadResolution(12);        // 0..4095
//   delay(3000);                     // brief warm-up; full burn-in is minutes

//   Serial1.println("\nMiCS analog read starting...");
//   Serial1.println("ADC_raw, V_adc");
// }

// void loop() {
//   // Oversample a bit for stability
//   const int N = 16;
//   uint32_t sum = 0;
//   for (int i = 0; i < N; i++) {
//     sum += analogRead(MICS_AO);
//     delay(3);
//   }
//   uint16_t adc = sum / N;
//   float v_adc = (adc / 4095.0f) * 3.3f;  // assuming 3.3 V ADC reference

//   // Simple rail check (warn if you’re hitting the 3.3 V ceiling)
//   if (adc > 4080) {
//     Serial1.println("WARNING: AO near/above 3.3 V (reading saturated).");
//   }

//   Serial1.print(adc);
//   Serial1.print(", ");
//   Serial1.println(v_adc, 3);

//   delay(100);
// }

// MiCS analog gas sensor with DFRobot_MICS on STM32 (IntroBus board)
// UART: Serial1 on PA10/PA9. ADC: PB0. EN pin: PC6.

#include <Arduino.h>
#include "DFRobot_MICS.h"

HardwareSerial Serial1(PA10, PA9);   // RX, TX (your style)

// ---- Pin map (match your header) ----
#define MICS_AO    PB0      // Analog output from MiCS
#define MICS_EN    PC6      // Enable / power control pin (HIGH = ON)

// Warm-up time in minutes (DFRobot lib expects minutes)
#define CALIBRATION_TIME 3

// Create ADC-mode sensor (analog pin + power/EN pin)
DFRobot_MICS_ADC mics(MICS_AO, MICS_EN);

void setup() {
  Serial1.begin(115200, SERIAL_8E1);
  delay(200);

  // 12-bit ADC on STM32 (0..4095, 3.3 V reference)
  analogReadResolution(12);

  // Start sensor
  while (!mics.begin()) {
    Serial1.println("MiCS not found — retrying...");
    delay(1000);
  }
  Serial1.println("MiCS sensor initialized.");

  // Wake if needed
  if (mics.getPowerState() == SLEEP_MODE) {
    mics.wakeUpMode();
    Serial1.println("Waking sensor...");
  } else {
    Serial1.println("Sensor already awake.");
  }

  // Warm-up / calibration
  Serial1.print("Warming up for ");
  Serial1.print(CALIBRATION_TIME);
  Serial1.println(" minute(s)...");
  while (!mics.warmUpTime(CALIBRATION_TIME)) {
    delay(1000);
  }
  Serial1.println("Warm-up complete!\n");

  Serial1.println("CO(ppm), NH3(ppm), H2(ppm), EtOH(ppm), CH4(ppm)");
}

void loop() {
  // Read all supported gases (per DFRobot_MICS API)
  float co   = mics.getGasData(CO);
  float nh3  = mics.getGasData(NH3);
  float h2   = mics.getGasData(H2);
  float etoh = mics.getGasData(C2H5OH);
  float ch4  = mics.getGasData(CH4);

  // Some boards may return <0 while settling; clamp to 0 for display
  if (co   < 0) co = 0;
  if (nh3  < 0) nh3 = 0;
  if (h2   < 0) h2 = 0;
  if (etoh < 0) etoh = 0;
  if (ch4  < 0) ch4 = 0;

  // CSV for easy logging
  Serial1.print(co,   2); Serial1.print(", ");
  Serial1.print(nh3,  2); Serial1.print(", ");
  Serial1.print(h2,   2); Serial1.print(", ");
  Serial1.print(etoh, 2); Serial1.print(", ");
  Serial1.println(ch4,2);

  delay(1000);
}


