
// Grove – UV Sensor (GUVA-S12D) — connected to A0
const int UV_PIN = A0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  const int samples = 1024;
  long sum = 0;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(UV_PIN);
    delay(2);
  }
  float avgReading = sum / float(samples);

  // Convert ADC reading (0–1023) to voltage (millivolts):
  // 5V reference assumed; change to 3.3 if needed
  float voltage_mV = avgReading * (5000.0 / 1023.0);

  // According to the datasheet:
  // illumination (in mW/m²) = 307 × Vsig (V)
  // Since voltage_mV is in mV, convert accordingly
  float voltage_V = voltage_mV / 1000.0;
  float illumination = 307.0 * voltage_V; // in mW/m²

  // Estimate UV Index:
  // UV Index ≈ illumination / 200
  float uvIndex = illumination / 200.0;

  Serial.print("Voltage: ");
  Serial.print(voltage_mV, 1);
  Serial.print(" mV — UV Intensity: ");
  Serial.print(illumination, 1);
  Serial.print(" mW/m² — Estimated UV Index: ");
  Serial.println(uvIndex, 2);

  delay(1000);
}



