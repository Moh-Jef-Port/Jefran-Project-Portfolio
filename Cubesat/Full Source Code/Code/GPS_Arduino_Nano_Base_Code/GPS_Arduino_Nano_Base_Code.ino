#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>

TinyGPSPlus gps;
SoftwareSerial gpsSerial(4, 3); // RX = D4 (to GPS TX), TX = D3 (to GPS RX)

unsigned long lastPrint = 0;

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600);
  Serial.println("GPS Module Test: Waiting for coordinates...");
}

void loop() {
  // Keep reading incoming GPS data
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Only print if location is updated
  if (gps.location.isUpdated()) {
    Serial.println("Location updated:");
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Satellites: ");
    Serial.println(gps.satellites.value());
    Serial.println("-----------------------");
  }

  // Optional: print message every 5 seconds if no fix yet
  if (millis() - lastPrint > 5000 && !gps.location.isValid()) {
    Serial.println("Still searching for GPS fix...");
    lastPrint = millis();
  }
}
