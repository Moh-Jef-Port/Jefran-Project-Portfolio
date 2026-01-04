// ===== AS7265x (Triad) standalone test for STM32H7 (stm32duino) =====
// Install: "SparkFun AS7265x" library (Library Manager).
// I2C: 3.3V logic only. AS7265x default addr = 0x49.

#include <Arduino.h>
#include <Wire.h>
#include "SparkFun_AS7265X.h"

AS7265X triad;

static const uint8_t GAIN_1X   = 0;
static const uint8_t GAIN_3_7X = 1;
static const uint8_t GAIN_16X  = 2;
static const uint8_t GAIN_64X  = 3;

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  delay(30);

  Wire.begin(); // use default I2C pins for your STM32 board

  if (!triad.begin(Wire)) {
    Serial.println("# ERROR: AS7265x not detected at 0x49. Check 3.3V, GND, SDA/SCL.");
    while (1) { delay(1000); }
  }

  // Quiet the blue indicator LED for cleaner measurements
  triad.disableIndicator();

  // Typical starting points (adjust if saturating / too low):
  triad.setGain(GAIN_16X);          // 1x/3.7x/16x/64x
  triad.setIntegrationCycles(50);   // 50 * 2.8ms ≈ 140ms per bank
  // You can also use triad.setMeasurementMode(...) but takeMeasurements() handles bank cycling.

  // Optional: use the on-board white bulb for reflectance measurements
  // 0=White, 1=IR, 2=UV (from SparkFun guide)
  triad.disableBulb(0); // ensure off at startup

  // CSV header
  Serial.println("ms,A410,B435,C460,D485,E510,F535,G560,H585,I645,J705,K900,L940,R610,S680,T730,U760,V810,W860");
}

void loop() {
  // Illuminate sample with white LED (set current separately if needed)
  triad.enableBulb(0);         // 0=White
  triad.takeMeasurements();    // one-shot, cycles banks to populate all channels
  triad.disableBulb(0);

  // Read calibrated irradiance (uW/cm^2) for all 18 channels (A..W mapping per datasheet)
  float A = triad.getCalibratedA();
  float B = triad.getCalibratedB();
  float C = triad.getCalibratedC();
  float D = triad.getCalibratedD();
  float E = triad.getCalibratedE();
  float F = triad.getCalibratedF();

  float G = triad.getCalibratedG();
  float H = triad.getCalibratedH();
  float I = triad.getCalibratedI();
  float J = triad.getCalibratedJ();
  float K = triad.getCalibratedK();
  float L = triad.getCalibratedL();

  float R = triad.getCalibratedR();
  float S = triad.getCalibratedS();
  float T = triad.getCalibratedT();
  float U = triad.getCalibratedU();
  float V = triad.getCalibratedV();
  float W = triad.getCalibratedW();

  unsigned long ms = millis();
  Serial.print(ms); Serial.print(',');
  Serial.print(A,3); Serial.print(','); Serial.print(B,3); Serial.print(',');
  Serial.print(C,3); Serial.print(','); Serial.print(D,3); Serial.print(',');
  Serial.print(E,3); Serial.print(','); Serial.print(F,3); Serial.print(',');
  Serial.print(G,3); Serial.print(','); Serial.print(H,3); Serial.print(',');
  Serial.print(I,3); Serial.print(','); Serial.print(J,3); Serial.print(',');
  Serial.print(K,3); Serial.print(','); Serial.print(L,3); Serial.print(',');
  Serial.print(R,3); Serial.print(','); Serial.print(S,3); Serial.print(',');
  Serial.print(T,3); Serial.print(','); Serial.print(U,3); Serial.print(',');
  Serial.print(V,3); Serial.print(','); Serial.println(W,3);

  delay(300); // ~3–4 Hz with the chosen integration; tune as needed
}
