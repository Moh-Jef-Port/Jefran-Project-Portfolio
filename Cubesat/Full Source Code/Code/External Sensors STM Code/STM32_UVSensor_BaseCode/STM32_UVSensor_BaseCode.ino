#include <Arduino.h>

HardwareSerial Serial1(PA10, PA9);     // your UART

// ---- Choose any ADC-capable pin from your header ----
#define UV_PIN PB0

// Set this to the ADC reference on your board (usually 3.3 V)
const float REF_VOLTAGE = 3.3f;        // volts

// GUVA-S12D transfer: Irradiance (mW/m^2) = 307 * Vsig(V)
const float UV_K = 307.0f;

// Heuristic: UV Index ≈ irradiance / 200
const float UVI_DIV = 200.0f;

// Oversampling
const int SAMPLES = 256;

void setup() {
  Serial1.begin(115200, SERIAL_8E1);
  analogReadResolution(12);            // 0..4095 for STM32
  delay(100);

  Serial1.println("\nGUVA-S12D UV Sensor — Labeled Output");
  Serial1.println("Labels: ADC, Voltage(mV), UV(mW/m^2), UVIndex");
}

void loop() {
  // Average multiple samples for stability
  uint32_t sum = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum += analogRead(UV_PIN);
    delay(2);
  }
  float adc_avg = sum / float(SAMPLES);

  // Convert to voltage at the ADC pin
  float v_adc = (adc_avg / 4095.0f) * REF_VOLTAGE;  // volts
  float v_mV  = v_adc * 1000.0f;                    // millivolts

  // Convert to irradiance and UV index
  float uv_mW_m2 = UV_K * v_adc;                    // mW/m^2
  float uv_index = uv_mW_m2 / UVI_DIV;

  // Labeled line
  Serial1.print("ADC: ");        Serial1.print((int)adc_avg);
  Serial1.print(", Voltage: ");  Serial1.print(v_mV, 1);      Serial1.print(" mV");
  Serial1.print(", UV: ");       Serial1.print(uv_mW_m2, 1);  Serial1.print(" mW/m^2");
  Serial1.print(", UVI: ");      Serial1.println(uv_index, 2);

  // Optional saturation warning
  if (adc_avg > 4080) {
    Serial1.println("WARNING: ADC near full scale—check wiring/supply.");
  }

  delay(500);
}
