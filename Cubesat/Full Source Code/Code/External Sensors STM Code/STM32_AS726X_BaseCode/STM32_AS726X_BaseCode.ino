#include <Arduino.h>
#include <Wire.h>
#include "AS726X.h"            // SparkFun AS726X (the library you pasted)

// ---- UART to your comms path ----
HardwareSerial Serial1(PA10, PA9);   // 115200, 8E1

// ---- Your custom I2C bus ----
TwoWire Wire4(PD13, PD12);           // SDA=PD13, SCL=PD12

// ---- I2C mux (TCA/PCA9548A) ----
#define MUX_ADDR 0x70

// ---- AS726x address ----
#define AS726X_ADDR 0x49

// Gain codes (library expects 0..3): 0=1x, 1=3.7x, 2=16x, 3=64x
#define AS726X_GAIN   2          // good starting point (16x)
// Measurement modes: 0/1=banked, 2=one-shot all, 3=continuous all
#define AS726X_MODE   3          // continuous all 6 channels
// Integration time units are ~2.8ms each. 50 ≈ 140ms
#define AS726X_INT    50

AS726X sensor;                    // sensor object (uses virtual regs via I2C)

// ---------------- MUX helpers ----------------
static void muxSelect(int8_t ch) {
  Wire4.beginTransmission(MUX_ADDR);
  Wire4.write((ch >= 0 && ch < 8) ? (1u << ch) : 0x00);
  Wire4.endTransmission();
  delay(2);
}
static bool i2cProbe(uint8_t addr) {
  Wire4.beginTransmission(addr);
  return (Wire4.endTransmission() == 0);
}
// Optional: quick scan per channel (debug)
static void debugScanChannel(uint8_t ch) {
  muxSelect(ch);
  bool any = false;
  for (uint8_t a = 3; a < 0x78; a++) {
    if (a == MUX_ADDR) continue;
    if (i2cProbe(a)) {
      if (!any) { Serial1.print("Ch "); Serial1.print(ch); Serial1.print(": "); any = true; }
      Serial1.print("0x"); if (a < 16) Serial1.print('0'); Serial1.print(a, HEX); Serial1.print(' ');
    }
  }
  if (any) Serial1.println();
}
static int8_t findAS726xOnMux() {
  for (uint8_t ch = 0; ch < 8; ch++) {
    muxSelect(ch);
    if (i2cProbe(AS726X_ADDR)) return ch;
  }
  return -1;
}

void setup() {
  Serial1.begin(115200, SERIAL_8E1);
  delay(100);

  Wire4.begin();
  Wire4.setClock(400000);                 // drop to 100k if long/noisy wires

  Serial1.println("\nAS726x auto-discovery via mux on Wire4 (PD13/PD12)");
  for (uint8_t ch = 0; ch < 8; ch++) debugScanChannel(ch);

  const int8_t ch = findAS726xOnMux();
  if (ch < 0) {
    Serial1.println("ERROR: AS726x (0x49) not found on any mux channel.");
    while (1) delay(500);
  }
  Serial1.print("Found AS726x at mux channel "); Serial1.println(ch);
  muxSelect(ch);

  // Initialize sensor on our custom bus with chosen gain/mode
  // (Signature per your library header: begin(TwoWire&, byte gain, byte measurementMode))
  sensor.begin(Wire4, AS726X_GAIN, AS726X_MODE);
  sensor.setIntegrationTime(AS726X_INT);   // ~140ms
  // Optional illumination/indicator control (uncomment if you wired LEDs):
  // sensor.enableIndicator();
  // sensor.enableBulb(); sensor.setBulbCurrent(3); // 0..3 => 12.5/25/50/100mA

  // CSV header
  if (sensor.getVersion() == SENSORTYPE_AS7262) {
    Serial1.println("Labels: sensor,ch,addr,V,B,G,Y,O,R,tempF");
  } else if (sensor.getVersion() == SENSORTYPE_AS7263) {
    Serial1.println("Labels: sensor,ch,addr,V,B,G,Y,O,R,tempF");
  } else {
    Serial1.println("WARN: Unknown AS726x version (defaulting to A..F labels)");
    Serial1.println("Labels: sensor,ch,addr,V,B,G,Y,O,R,tempF");
  }
}

void loop() {
  // In continuous mode, new data appears periodically. If you pick one-shot (mode=2),
  // call sensor.takeMeasurements() each loop instead.
  if (!sensor.dataAvailable()) { delay(5); return; }

  const byte ver = sensor.getVersion();
  float tF = sensor.getTemperatureF();

  Serial1.print("AS726x, ");
  // We don't store the mux channel globally; read back the active mask for clarity if needed,
  // but since we locked the mux once in setup, just report '0x49' and 'ch=?' is optional.
  Serial1.print("?, ");                 // put your known channel number here if you want
  Serial1.print("0x49, ");

  if (ver == SENSORTYPE_AS7262) {
    // Calibrated VIS (V,B,G,Y,O,R)
    Serial1.print(sensor.getCalibratedViolet(), 3); Serial1.print(", ");
    Serial1.print(sensor.getCalibratedBlue(),   3); Serial1.print(", ");
    Serial1.print(sensor.getCalibratedGreen(),  3); Serial1.print(", ");
    Serial1.print(sensor.getCalibratedYellow(), 3); Serial1.print(", ");
    Serial1.print(sensor.getCalibratedOrange(), 3); Serial1.print(", ");
    Serial1.print(sensor.getCalibratedRed(),    3); Serial1.print(", ");
  } else if (ver == SENSORTYPE_AS7263) {
    // Calibrated NIR (R,S,T,U,V,W)
    Serial1.print(sensor.getCalibratedR(), 3); Serial1.print(", ");
    Serial1.print(sensor.getCalibratedS(), 3); Serial1.print(", ");
    Serial1.print(sensor.getCalibratedT(), 3); Serial1.print(", ");
    Serial1.print(sensor.getCalibratedU(), 3); Serial1.print(", ");
    Serial1.print(sensor.getCalibratedV(), 3); Serial1.print(", ");
    Serial1.print(sensor.getCalibratedW(), 3); Serial1.print(", ");
  } else {
    // Fallback: if version check fails, try A..F accessors (some library variants use these)
    Serial1.print(sensor.getCalibratedViolet(), 3); Serial1.print(", ");
    Serial1.print(sensor.getCalibratedBlue(),   3); Serial1.print(", ");
    Serial1.print(sensor.getCalibratedGreen(),  3); Serial1.print(", ");
    Serial1.print(sensor.getCalibratedYellow(), 3); Serial1.print(", ");
    Serial1.print(sensor.getCalibratedOrange(), 3); Serial1.print(", ");
    Serial1.print(sensor.getCalibratedRed(),    3); Serial1.print(", ");
  }

  Serial1.println(tF, 1);   // tempF at end of line

  // Pace slightly faster than integration time; continuous mode will refresh itself
  delay(20);
}