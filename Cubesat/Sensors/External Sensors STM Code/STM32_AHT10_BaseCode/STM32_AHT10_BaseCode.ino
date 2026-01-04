#include <Arduino.h>
#include <Wire.h>

HardwareSerial Serial1(PA10, PA9);
TwoWire Wire4(PD13, PD12);

// ---- I2C mux ----
#define MUX_ADDR 0x70

// ---- AHTx0 common ----
#define AHT_ADDR       0x38
#define AHT_CMD_RESET  0xBA
#define AHT_CMD_TRIG   0xAC   // then 0x33, 0x00

// AHT10-specific init
#define AHT10_CMD_INIT 0xE1   // then 0x08, 0x00

// AHT20/AHT21 init (some variants accept 0xBE 0x08 0x00; others 0xBE 0x80 0x00)
#define AHT20_CMD_INIT 0xBE

static int8_t  g_ch   = -1;
static uint8_t g_addr = 0;

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
static bool ahtWrite3(uint8_t cmd, uint8_t b1, uint8_t b2) {
  Wire4.beginTransmission(g_addr);
  Wire4.write(cmd);
  Wire4.write(b1);
  Wire4.write(b2);
  return (Wire4.endTransmission() == 0);
}
static bool ahtReset() {
  Wire4.beginTransmission(g_addr);
  Wire4.write(AHT_CMD_RESET);
  if (Wire4.endTransmission() != 0) return false;
  delay(30); // give it time after reset
  return true;
}
static bool ahtTrigger() {
  // AC 33 00
  return ahtWrite3(AHT_CMD_TRIG, 0x33, 0x00);
}
static bool ahtRead6(uint8_t buf[6]) {
  // poll busy bit (MSB of first byte) up to ~120 ms
  uint32_t t0 = millis();
  while (millis() - t0 < 120) {
    if (Wire4.requestFrom((int)g_addr, 6) == 6) {
      for (int i=0;i<6;i++) buf[i] = Wire4.read();
      if ((buf[0] & 0x80) == 0) return true; // not busy
    }
    delay(5);
  }
  return false;
}

// Try an init sequence and verify by doing one measurement
static bool ahtTryInitAndTest(uint8_t initCmd, uint8_t b1, uint8_t b2) {
  if (!ahtWrite3(initCmd, b1, b2)) return false;
  delay(10);
  if (!ahtTrigger()) return false;
  uint8_t buf[6];
  if (!ahtRead6(buf)) return false;

  // decode once to ensure sane numbers
  uint32_t rawHum = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | ((uint32_t)buf[3] >> 4);
  uint32_t rawTmp = (((uint32_t)buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | (uint32_t)buf[5];

  float rh = (rawHum * 100.0f) / 1048576.0f;
  float tC = (rawTmp * 200.0f) / 1048576.0f - 50.0f;
  (void)rh; (void)tC; // just verifying flow; real prints in loop
  return true;
}

static bool findAHT() {
  for (uint8_t ch=0; ch<8; ch++) {
    muxSelect(ch);
    if (i2cProbe(AHT_ADDR)) { g_ch = ch; g_addr = AHT_ADDR; return true; }
  }
  return false;
}

enum AHTType { AHT_UNKNOWN, AHT10, AHT20 };
static AHTType which = AHT_UNKNOWN;

void setup() {
  Serial1.begin(115200, SERIAL_8E1);
  delay(120);
  Wire4.begin();
  Wire4.setClock(400000); // drop to 100k if long/noisy

  Serial1.println("\nAHTx0 via TCA/PCA9548A (Wire4 PD13/PD12)");

  if (!findAHT()) {
    Serial1.println("ERROR: No AHT10/20 (0x38) found on any mux channel.");
    while (1) { delay(500); }
  }
  Serial1.print("Found AHTx0 on mux channel "); Serial1.print(g_ch);
  Serial1.print(", addr 0x"); Serial1.println(g_addr, HEX);

  muxSelect(g_ch);
  if (!ahtReset()) {
    Serial1.println("ERROR: AHT reset failed.");
    while (1) { delay(500); }
  }

  // Try AHT10 init first (E1 08 00)
  if (ahtTryInitAndTest(AHT10_CMD_INIT, 0x08, 0x00)) {
    which = AHT10;
    Serial1.println("Init path: AHT10 (E1 08 00)");
  } else {
    // Try AHT20 variants
    if (ahtTryInitAndTest(AHT20_CMD_INIT, 0x08, 0x00)) {
      which = AHT20;
      Serial1.println("Init path: AHT20 (BE 08 00)");
    } else if (ahtTryInitAndTest(AHT20_CMD_INIT, 0x80, 0x00)) {
      which = AHT20;
      Serial1.println("Init path: AHT20 (BE 80 00)");
    } else {
      Serial1.println("ERROR: AHT init failed (tried AHT10 & AHT20 sequences).");
      while (1) { delay(500); }
    }
  }

  Serial1.println("Labels: sensor, ch, addr, RH(%), Temp(C)");
}

void loop() {
  muxSelect(g_ch);

  if (!ahtTrigger()) { Serial1.println("trigger error"); delay(500); return; }

  uint8_t buf[6];
  if (!ahtRead6(buf)) { Serial1.println("read timeout"); delay(500); return; }

  uint32_t rawHum = ((uint32_t)buf[1] << 12) |
                    ((uint32_t)buf[2] << 4)  |
                    ((uint32_t)buf[3] >> 4);
  uint32_t rawTmp = (((uint32_t)buf[3] & 0x0F) << 16) |
                    ((uint32_t)buf[4] << 8) |
                    (uint32_t)buf[5];

  float rh = (rawHum * 100.0f) / 1048576.0f;
  float tC = (rawTmp * 200.0f) / 1048576.0f - 50.0f;

  if (rh < 0) rh = 0; if (rh > 100) rh = 100;

  Serial1.print((which==AHT10) ? "AHT10, " : "AHT20, ");
  Serial1.print(g_ch);
  Serial1.print(", 0x"); Serial1.print(g_addr, HEX);
  Serial1.print(", ");   Serial1.print(rh, 1);
  Serial1.print(", ");   Serial1.println(tC, 2);

  delay(500);
}
