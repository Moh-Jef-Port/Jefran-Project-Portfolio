// ===== COMBINED LOGGER: AHT10 + GUVA-S12D + MiCS-5524 + AS7265x (STM32H7) =====
// Install libs: "Adafruit AHTX0" and "SparkFun AS7265x" (Library Manager).
// I2C: AHT10 + AS7265x on Wire (3.3V). Analog: GUVA SIG -> A0, MiCS sense -> A1.
//
// References:
// - AS7265x addr 0x49; functions begin(), takeMeasurements(), getCalibratedA()..W(), enableBulb() 0/1/2.  :contentReference[oaicite:5]{index=5}
// - 18-channel wavelength map A..W per datasheet.  :contentReference[oaicite:6]{index=6}
// - GUVA-S12D formula: Intensity(mW/m^2) = 307 * Vsig; UV Index ≈ Intensity/200.  (We log volts + can derive UVI client-side.) :contentReference[oaicite:7]{index=7}
// - MiCS-5524 divider math: Rs = RLOAD * (Vsup - Vout) / Vout; ensure ADC < 3.3V (divider if needed).

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include "SparkFun_AS7265X.h"

// ------------------- USER PINS -------------------
static const int   UV_PIN   = A0;   // GUVA-S12D SIG
static const int   MICS_PIN = A1;   // MiCS sense node (across RLOAD into divider/ADC)
// -------------------------------------------------

// ------------- ELECTRICAL / ADC CONSTANTS --------
static const uint16_t ADC_BITS = 12;    // STM32 default
static const float    ADC_REF_V = 3.3f; // VDDA
// MiCS sensing bridge assumptions:
static const float    V_SUPPLY  = 5.0f;     // MiCS divider supply
static const float    R_LOAD    = 10000.0f; // ohms (>= 820Ω; often 10k–47k)
// If you added a divider to protect the ADC (recommended), scale back:
// Example: 100k/100k divider => ADC sees 1/2 node => set gain to 2.0f.
static const float    MICS_DIVIDER_GAIN = 1.0f;
// -------------------------------------------------

// ------------- SMOOTHING -------------------------
static const float EMA_UV_ALPHA   = 0.15f;
static const float EMA_MICS_ALPHA = 0.10f;
// Optional slow R0 drift update (only when near ambient)
static const float R0_SLOW_ALPHA  = 0.001f;
static const float R0_UPDATE_MIN_RATIO = 0.85f;
static const float R0_UPDATE_MAX_RATIO = 1.20f;
// -------------------------------------------------

Adafruit_AHTX0 aht;
AS7265X       triad;

static float g_uv_v_ema   = NAN;
static float g_mics_v_ema = NAN;
static float g_R0         = NAN;

static inline float clip(float x, float lo, float hi) {
  return x < lo ? lo : (x > hi ? hi : x);
}

float analogVoltage(int pin, uint16_t oversample = 16) {
  uint32_t acc = 0;
  for (uint16_t i=0; i<oversample; ++i) { acc += analogRead(pin); delay(2); }
  float raw = acc / float(oversample);
  return (raw / ((1u << ADC_BITS) - 1)) * ADC_REF_V;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  delay(40);

  analogReadResolution(ADC_BITS);

  // -------- AHT10 --------
  Wire.begin();
  if (!aht.begin()) {
    Serial.println("# ERROR: AHT10 not found. Check I2C wiring.");
    while (1) { delay(1000); }
  }

  // -------- AS7265x (Triad) --------
  if (!triad.begin(Wire)) {
    Serial.println("# ERROR: AS7265x not found at 0x49. Check I2C wiring (3.3V).");
    while (1) { delay(1000); }
  }
  triad.disableIndicator();
  triad.setGain(2);                // 0:1x, 1:3.7x, 2:16x, 3:64x
  triad.setIntegrationCycles(50);  // 50 * 2.8ms ≈ 140ms/bank
  triad.disableBulb(0);            // white off

  // -------- Seed EMAs --------
  g_uv_v_ema   = analogVoltage(UV_PIN);
  g_mics_v_ema = analogVoltage(MICS_PIN) * MICS_DIVIDER_GAIN;

  // -------- MiCS baseline (R0) --------
  {
    const uint16_t N = 200;
    float sumRs = 0;
    for (uint16_t i=0; i<N; ++i) {
      float v_node = analogVoltage(MICS_PIN) * MICS_DIVIDER_GAIN;
      v_node = clip(v_node, 0.001f, V_SUPPLY - 0.001f);
      float Rs = R_LOAD * (V_SUPPLY - v_node) / v_node;
      sumRs += Rs;
      delay(10);
    }
    g_R0 = sumRs / N;
  }

  // -------- CSV header --------
  Serial.println(
    "ms,temp_C,rel_humidity_pct,"
    "uv_v,uv_v_ema,"
    "mics_v,mics_v_ema,Rs_ohm,Rs_over_R0,"
    "A410,B435,C460,D485,E510,F535,G560,H585,I645,J705,K900,L940,R610,S680,T730,U760,V810,W860"
  );
}

void loop() {
  // AHT10
  sensors_event_t hum, temp;
  aht.getEvent(&hum, &temp);

  // GUVA-S12D (voltage + EMA)
  float uv_v = analogVoltage(UV_PIN);
  if (isnan(g_uv_v_ema)) g_uv_v_ema = uv_v;
  g_uv_v_ema = (1.0f - EMA_UV_ALPHA) * g_uv_v_ema + EMA_UV_ALPHA * uv_v;

  // MiCS-5524 (EMA + safe math)
  float mics_v = analogVoltage(MICS_PIN) * MICS_DIVIDER_GAIN;
  mics_v = clip(mics_v, 0.001f, V_SUPPLY - 0.001f);
  if (isnan(g_mics_v_ema)) g_mics_v_ema = mics_v;
  g_mics_v_ema = (1.0f - EMA_MICS_ALPHA) * g_mics_v_ema + EMA_MICS_ALPHA * mics_v;
  float Rs = R_LOAD * (V_SUPPLY - g_mics_v_ema) / g_mics_v_ema;
  float ratio = Rs / g_R0;
  if (ratio >= R0_UPDATE_MIN_RATIO && ratio <= R0_UPDATE_MAX_RATIO && R0_SLOW_ALPHA > 0.0f) {
    g_R0 = (1.0f - R0_SLOW_ALPHA) * g_R0 + R0_SLOW_ALPHA * Rs;
    ratio = Rs / g_R0;
  }

  // AS7265x (white bulb reflectance one-shot)
  triad.enableBulb(0);
  triad.takeMeasurements();
  triad.disableBulb(0);

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
  // AHT10
  Serial.print(temp.temperature, 2); Serial.print(',');
  Serial.print(hum.relative_humidity, 1); Serial.print(',');
  // UV
  Serial.print(uv_v, 3); Serial.print(',');
  Serial.print(g_uv_v_ema, 3); Serial.print(',');
  // MiCS
  Serial.print(mics_v, 3); Serial.print(',');
  Serial.print(g_mics_v_ema, 3); Serial.print(',');
  Serial.print(Rs, 0); Serial.print(',');
  Serial.print(ratio, 3); Serial.print(',');
  // AS7265x channels (uW/cm^2)
  Serial.print(A,3); Serial.print(','); Serial.print(B,3); Serial.print(',');
  Serial.print(C,3); Serial.print(','); Serial.print(D,3); Serial.print(',');
  Serial.print(E,3); Serial.print(','); Serial.print(F,3); Serial.print(',');
  Serial.print(G,3); Serial.print(','); Serial.print(H,3); Serial.print(',');
  Serial.print(I,3); Serial.print(','); Serial.print(J,3); Serial.print(',');
  Serial.print(K,3); Serial.print(','); Serial.print(L,3); Serial.print(',');
  Serial.print(R,3); Serial.print(','); Serial.print(S,3); Serial.print(',');
  Serial.print(T,3); Serial.print(','); Serial.print(U,3); Serial.print(',');
  Serial.print(V,3); Serial.print(','); Serial.println(W,3);

  // With ~140ms/bank and 3 banks per takeMeasurements() you’ll be ~0.7–1.0 Hz depending on USB/printf.
  delay(50);
}
