// STM32H7 + MiCS-5524 analog output
#include <Arduino.h>

// ====== USER CONFIG ======
static const int MICS_PIN = A1;       // analog pin reading the voltage across RLOAD
static const float V_SUPPLY = 5.0f;   // sensing bridge supply (typ breakout uses 5V)
static const float R_LOAD  = 10000.0f; // your load resistor in ohms (>= 820Ω; many boards use 10k–47k)
// =========================

static const uint16_t ADC_BITS = 12;  // STM32 default 12-bit
static const float ADC_REF_V = 3.3f;  // STM32 ADC reference (VDDA)

float R0 = NAN; // baseline Rs in clean air

float readVoltage(int pin) {
  const uint16_t N = 16;
  uint32_t acc = 0;
  for (uint16_t i=0; i<N; ++i) { acc += analogRead(pin); delay(2); }
  float raw = acc / float(N);
  return (raw / ((1u << ADC_BITS) - 1)) * ADC_REF_V;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  analogReadResolution(ADC_BITS);
  delay(50);

  Serial.println("MiCS-5524 warming/heating... ensure heater is powered via 82Ω from 5V.");
  // Quick baseline in ambient air (for field use; lab-grade wants >24–48 h burn-in)
  const uint16_t samples = 200;
  float sumRs = 0;
  for (uint16_t i=0; i<samples; ++i) {
    float v_out = readVoltage(MICS_PIN);     // voltage across RLOAD
    v_out = min(max(v_out, 0.001f), ADC_REF_V); // avoid div-by-zero
    // Divider: Vout = Vsup * (RLOAD / (RLOAD + Rs))  =>  Rs = RLOAD * (Vsup - Vout) / Vout
    float Rs = R_LOAD * (V_SUPPLY - v_out) / v_out;
    sumRs += Rs;
    delay(10);
  }
  R0 = sumRs / samples;
  Serial.print("Baseline R0 (ambient clean air) = ");
  Serial.print(R0, 0); Serial.println(" ohms");
}

void loop() {
  float v_out = readVoltage(MICS_PIN);
  v_out = min(max(v_out, 0.001f), ADC_REF_V);

  float Rs = R_LOAD * (V_SUPPLY - v_out) / v_out;
  float ratio = Rs / R0;  // Rs/R0 curves are what the datasheet provides

  Serial.print("Vout="); Serial.print(v_out, 3); Serial.print(" V, ");
  Serial.print("Rs="); Serial.print(Rs, 0); Serial.print(" Ω, ");
  Serial.print("Rs/R0="); Serial.println(ratio, 3);

  // You can use the Rs/R0 vs ppm curves in the datasheet to estimate ppm for specific gases.
  // (Different gases change Rs differently; this is NOT a selective sensor.)
  delay(500);
}
