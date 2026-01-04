#include <Arduino.h>
#include <Wire.h>

HardwareSerial Serial1(PA10, PA9);

// ---- I2C bus on your custom pins ----
TwoWire Wire4(PD13, PD12);

// ---- Mux address ----
#define MUX_ADDR 0x70

// ---- BH1750 addresses & commands ----
#define BH1750_ADDR_LOW   0x23
#define BH1750_ADDR_HIGH  0x5C
#define BH1750_POWER_ON   0x01
#define BH1750_RESET      0x07
#define BH1750_CONT_HIRES 0x10

// Globals set after discovery
static uint8_t g_bhAddr = 0;
static int8_t  g_bhCh   = -1;

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

// OPTIONAL: quick per-channel scan (debug helper)
static void debugScanChannel(uint8_t ch) {
  muxSelect(ch);
  bool any = false;
  for (uint8_t a = 3; a < 0x78; a++) {
    if (a == MUX_ADDR) continue;
    if (i2cProbe(a)) {
      if (!any) { Serial1.print("Ch "); Serial1.print(ch); Serial1.print(": "); any = true; }
      Serial1.print("0x");
      if (a < 16) Serial1.print('0');
      Serial1.print(a, HEX);
      Serial1.print(' ');
    }
  }
  if (any) Serial1.println();
}

// Find BH1750 across channels; set g_bhCh & g_bhAddr
static bool findBH1750() {
  for (uint8_t ch = 0; ch < 8; ch++) {
    muxSelect(ch);
    // Try default then ALT address
    if (i2cProbe(BH1750_ADDR_LOW))  { g_bhCh = ch; g_bhAddr = BH1750_ADDR_LOW;  return true; }
    if (i2cProbe(BH1750_ADDR_HIGH)) { g_bhCh = ch; g_bhAddr = BH1750_ADDR_HIGH; return true; }
  }
  return false;
}

static bool bh1750Write(uint8_t cmd) {
  Wire4.beginTransmission(g_bhAddr);
  Wire4.write(cmd);
  return (Wire4.endTransmission() == 0);
}

static bool bh1750Init() {
  // Ensure the correct branch is active
  muxSelect(g_bhCh);

  if (!bh1750Write(BH1750_POWER_ON)) return false;
  delay(5);
  bh1750Write(BH1750_RESET);  // optional
  delay(5);
  if (!bh1750Write(BH1750_CONT_HIRES)) return false;
  delay(180); // first conversion latency
  return true;
}

static bool bh1750ReadLux(float &lux) {
  muxSelect(g_bhCh);
  if (Wire4.requestFrom((int)g_bhAddr, 2) != 2) return false;
  uint16_t raw = ((uint16_t)Wire4.read() << 8) | Wire4.read();
  lux = raw / 1.2f; // high-res mode conversion
  return true;
}

void setup() {
  Serial1.begin(115200, SERIAL_8E1);
  delay(100);

  Wire4.begin();
  Wire4.setClock(400000); // 100k if your wiring is long/noisy

  Serial1.println("\nBH1750 auto-discovery via TCA/PCA9548A (Wire4 PD13/PD12)");

  // (Optional) show what's on each channel to spot that 0x49 device
  for (uint8_t ch = 0; ch < 8; ch++) debugScanChannel(ch);

  if (!findBH1750()) {
    Serial1.println("ERROR: No BH1750 found on any mux channel (0x23/0x5C).");
    Serial1.println("Tip: If you only see 0x49, that is NOT BH1750 (likely TSL2561 or ADS1115).");
    while (1) { delay(500); }
  }

  Serial1.print("Found BH1750 at channel ");
  Serial1.print(g_bhCh);
  Serial1.print(", address 0x");
  Serial1.println(g_bhAddr, HEX);

  if (!bh1750Init()) {
    Serial1.println("ERROR: BH1750 init failed.");
    while (1) { delay(500); }
  }

  Serial1.println("Labels: sensor, channel, addr, lux");
}

void loop() {
  float lux = 0.0f;
  if (bh1750ReadLux(lux)) {
    Serial1.print("BH1750, ");
    Serial1.print(g_bhCh);
    Serial1.print(", 0x");
    Serial1.print(g_bhAddr, HEX);
    Serial1.print(", ");
    Serial1.println(lux, 1);
  } else {
    Serial1.println("read error");
  }
  delay(200);
}
