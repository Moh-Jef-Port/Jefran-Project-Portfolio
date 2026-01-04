// STM32H7 + Grove UV (GUVA-S12D)
#include <Arduino.h>

static const float ADC_REF_V = 3.3f;  // VDDA on your STM32 board
static const uint16_t ADC_BITS = 12;  // STM32 default is 12-bit
static const int UV_PIN = A0;         // change to your actual analog pin

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  analogReadResolution(ADC_BITS);
  delay(50);
  Serial.println("Grove UV ready.");
}

void loop() {
  // Oversample a bit for stability
  const uint16_t N = 16;
  uint32_t acc = 0;
  for (uint16_t i=0; i<N; ++i) {
    acc += analogRead(UV_PIN);
    delay(2);
  }
  float raw = acc / float(N);
  float vSig = (raw / ((1u << ADC_BITS) - 1)) * ADC_REF_V;
  float intensity_mW_m2 = 307.0f * vSig;      // Seeed formula
  float uvIndex = intensity_mW_m2 / 200.0f;   // rough UV index

  Serial.print("raw="); Serial.print(raw, 1);
  Serial.print(", Vsig="); Serial.print(vSig, 3); Serial.print(" V, ");
  Serial.print("Intensity="); Serial.print(intensity_mW_m2, 1); Serial.print(" mW/m^2, ");
  Serial.print("UVI≈"); Serial.println(uvIndex, 2);

  delay(500);
}
