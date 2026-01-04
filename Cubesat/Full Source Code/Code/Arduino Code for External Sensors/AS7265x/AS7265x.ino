#include <Wire.h>
#include "Adafruit_AS726x.h"


Adafruit_AS726x as726x;


void setup() {
  Serial.begin(9600);
  Wire.begin();


  if (!as726x.begin()) {
    Serial.println("AS726x NOT detected. Check wiring + pullups.");
    while (1);
  }


  Serial.println("AS726x detected!");
}


void loop() {
  as726x.startMeasurement();
  delay(500);  // Wait for data ready (skip INT for now)


  float readings[6];
  as726x.readCalibratedValues(readings);


  Serial.print("V: "); Serial.print(readings[0], 2);
  Serial.print("\tB: "); Serial.print(readings[1], 2);
  Serial.print("\tG: "); Serial.print(readings[2], 2);
  Serial.print("\tY: "); Serial.print(readings[3], 2);
  Serial.print("\tO: "); Serial.print(readings[4], 2);
  Serial.print("\tR: "); Serial.println(readings[5], 2);


  delay(1000);
}




