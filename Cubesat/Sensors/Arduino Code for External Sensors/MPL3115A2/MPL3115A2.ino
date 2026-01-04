#include <Wire.h>
#include <Adafruit_MPL3115A2.h>


#define INT_PIN 6  // INT from MPL3115A2 to D6


Adafruit_MPL3115A2 mpl = Adafruit_MPL3115A2();


void setup() {
  Serial.begin(9600);
  Wire.begin();


  pinMode(INT_PIN, INPUT);  // Set INT pin for polling


  if (!mpl.begin()) {
    Serial.println("MPL3115A2 not detected. Check I2C wiring and logic level shifting.");
    while (1);
  }


  // Optional: print header for CSV format
  Serial.println("time(ms),altitude(m),pressure(Pa),temperature(C)");
}


void loop() {
  // Wait for sensor to signal data ready
  while (digitalRead(INT_PIN) == HIGH);  // Wait for INT to go LOW


  // Read sensor data
  float altitude = mpl.getAltitude();
  float pressure = mpl.getPressure();
  float temperature = mpl.getTemperature();


  // Output in CSV format
  Serial.print(millis());
  Serial.print(",");
  Serial.print(altitude, 2);
  Serial.print(",");
  Serial.print(pressure, 2);
  Serial.print(",");
  Serial.println(temperature, 2);


  delay(500);  // Optional delay between samples



