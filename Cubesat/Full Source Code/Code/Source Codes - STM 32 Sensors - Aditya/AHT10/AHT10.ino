// STM32H7 + AHT10 (Adafruit_AHTX0)
// Install: Adafruit AHTX0, Adafruit BusIO, Adafruit Unified Sensor
#include <Wire.h>
#include <Adafruit_AHTX0.h>

Adafruit_AHTX0 aht;

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  delay(50);

  // On STM32, default I2C is Wire (SCL/SDA per your variant)
  Wire.begin();       // If you use alternate pins, use Wire.setSCL()/setSDA() before begin()
  delay(10);

  if (!aht.begin()) {
    Serial.println("AHT10 not found. Check wiring and 3.3V/I2C pull-ups.");
    while (1) { delay(1000); }
  }
  Serial.println("AHT10 ready.");
}

void loop() {
  sensors_event_t hum, temp;
  aht.getEvent(&hum, &temp); // blocks ~80 ms for a fresh read
  Serial.print("T = "); Serial.print(temp.temperature, 2); Serial.print(" °C,  ");
  Serial.print("RH = "); Serial.print(hum.relative_humidity, 1); Serial.println(" %");
  delay(500);
}
