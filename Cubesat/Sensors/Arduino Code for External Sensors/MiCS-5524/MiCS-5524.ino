NOTE: Download library: https://github.com/DFRobot/DFRobot_MICS.git 

#include "DFRobot_MICS.h"

#define CALIBRATION_TIME 3  // Warm-up time in minutes
#define ADC_PIN A0
#define POWER_PIN 10

DFRobot_MICS_ADC mics(ADC_PIN, POWER_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  while (!mics.begin()) {
    Serial.println("Sensor not found—retrying...");
    delay(1000);
  }
  Serial.println("Sensor initialized.");

  if (mics.getPowerState() == SLEEP_MODE) {
    mics.wakeUpMode();
    Serial.println("Waking up sensor...");
  } else {
    Serial.println("Sensor already awake.");
  }

  Serial.println("Warming up, please wait...");
  while (!mics.warmUpTime(CALIBRATION_TIME)) {
    delay(1000);
  }
  Serial.println("Warm-up complete!");
}

void loop() {
  float co   = mics.getGasData(CO);
  float nh3  = mics.getGasData(NH3);
  float h2   = mics.getGasData(H2);
  float eth  = mics.getGasData(C2H5OH);
  float ch4  = mics.getGasData(CH4);

  Serial.print("CO: ");   Serial.print(co);   Serial.print(" ppm | ");
  Serial.print("NH3: ");  Serial.print(nh3);  Serial.print(" ppm | ");
  Serial.print("H2: ");   Serial.print(h2);   Serial.print(" ppm | ");
  Serial.print("EtOH: "); Serial.print(eth);  Serial.print(" ppm | ");
  Serial.print("CH4: ");  Serial.print(ch4);  Serial.println(" ppm");

  delay(1000);
}

