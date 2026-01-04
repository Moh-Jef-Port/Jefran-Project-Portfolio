//Note: [SDA -> A4, SCL -> A5]

#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter;

void setup() {
  Serial.begin(9600);
  Wire.begin(); // Join I2C bus with SDA=A4, SCL=A5

  // Start the BH1750 sensor
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750 sensor ready");
  } else {
    Serial.println("Error: BH1750 not detected");
    while (1); // Stop if sensor is not found
  }
}

void loop() {
  float lux = lightMeter.readLightLevel(); // Read lux value
  Serial.print("Light Level: ");
  Serial.print(lux);
  Serial.println(" lx");
  delay(1000); // 1 second delay
}

