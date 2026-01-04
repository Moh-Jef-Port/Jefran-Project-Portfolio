// #include <Wire.h>

// HardwareSerial Serial1(PA10, PA9);

// // TCA/PCA9548A default address; change if A0..A2 strapped (0x70–0x77)
// uint8_t MUX_ADDR = 0x70;

// void muxSelect(int8_t ch) {
//   // ch = 0..7 selects one channel; any other value disables all
//   Wire.beginTransmission(MUX_ADDR);
//   Wire.write((ch >= 0 && ch < 8) ? (1 << ch) : 0x00);
//   Wire.endTransmission();
//   delay(2);
// }

// void scanDownstream() {
//   byte error;
//   int nDevices = 0;

//   for (byte address = 1; address < 127; address++) {
//     if (address == MUX_ADDR) continue; // skip the mux itself
//     Wire.beginTransmission(address);
//     error = Wire.endTransmission();

//     if (error == 0) {
//       Serial1.print("I2C device found at address 0x");
//       if (address < 16) Serial1.print("0");
//       Serial1.println(address, HEX);
//       nDevices++;
//     } else if (error == 4) {
//       Serial1.print("Unknown error at address 0x");
//       if (address < 16) Serial1.print("0");
//       Serial1.println(address, HEX);
//     }
//   }

//   if (nDevices == 0) Serial1.println("  (none)");
// }

// void setup() {
//   Wire.setSDA(PD13);
//   Wire.setSCL(PD12);
//   Serial1.begin(115200, SERIAL_8E1);
//   Wire.begin();
//   Serial1.println("\nI2C MUX Scanner");
// }

// void loop() {
//   // Show the mux on the upstream bus
//   Serial1.println("Upstream scan:");
//   for (byte address = 1; address < 127; address++) {
//     Wire.beginTransmission(address);
//     if (Wire.endTransmission() == 0) {
//       Serial1.print("I2C device found at address 0x");
//       if (address < 16) Serial1.print("0");
//       Serial1.println(address, HEX);
//     }
//   }

//   // Scan each mux channel for downstream devices
//   for (int ch = 0; ch < 8; ch++) {
//     Serial1.print("\nScanning MUX channel ");
//     Serial1.println(ch);
//     muxSelect(ch);
//     scanDownstream();
//   }

//   // Disable all channels when done
//   muxSelect(-1);
//   Serial1.println("\nDone\n");

//   delay(5000);
// }

// ============================================================================
// STM32 External Sensors via TCA/PCA9548A (Wire PD13/PD12) — Serial1-only
// CSV logging every 5 seconds with a single header row
// MiCS-5524 warmup is non-blocking with countdown lines
// ============================================================================

#include <Arduino.h>
#include <Wire.h>
#include <math.h>

HardwareSerial Serial1(PA10, PA9);

// --------- CONFIG ----------
static const uint32_t LOG_INTERVAL_MS = 5000;  // emit one CSV row every 5 s

// --------- I2C MUX ----------
#define MUX_ADDR 0x70
static void muxSelect(int8_t ch) {
  Wire.beginTransmission(MUX_ADDR);
  Wire.write((ch >= 0 && ch < 8) ? (1u << ch) : 0x00);
  Wire.endTransmission();
  delay(2);
}
static bool i2cProbe(uint8_t addr) { Wire.beginTransmission(addr); return (Wire.endTransmission() == 0); }

// --------- timestamp ----------
static inline uint32_t now_ms(){ return millis(); }

// ============================================================================
// GUVA-S12D (Analog PB1)  -> mV
// ============================================================================
#ifndef GUVA_AO
#define GUVA_AO PB1
#endif
static void guvaInit(){ analogReadResolution(12); }
static bool guvaRead(float &mv){ int raw=analogRead(GUVA_AO); mv=(raw*3300.0f)/4095.0f; return true; }

// ============================================================================
// MiCS-5524 (Analog PB0 + EN PC6) — non-blocking warmup + countdown
// ============================================================================
#include "DFRobot_MICS.h"
#ifndef MICS_AO
#define MICS_AO PB0
#endif
#ifndef MICS_EN
#define MICS_EN PC6
#endif
#define MICS_WARMUP_MIN 3
static const uint32_t MICS_WARMUP_MS = (uint32_t)MICS_WARMUP_MIN * 60000UL;

DFRobot_MICS_ADC g_mics(MICS_AO, MICS_EN);
static bool     g_micsPresent=false, g_micsReady=false, g_micsWarming=false;
static uint32_t g_micsT0=0, g_lastWarmMsgMs=0;

static void micsInitNonBlocking() {
  analogReadResolution(12);
  if (!g_mics.begin()) { // try once; skip if absent
    Serial1.println("INFO,MiCS,not_detected");
    g_micsPresent=false; g_micsReady=false; g_micsWarming=false; return;
  }
  g_micsPresent=true;
  if (g_mics.getPowerState()==SLEEP_MODE) g_mics.wakeUpMode();
  g_micsWarming = true; g_micsReady=false; g_micsT0=millis(); g_lastWarmMsgMs=0;
}
static void micsServiceWarmup() {
  if (!g_micsPresent || g_micsReady==true || !g_micsWarming) return;
  uint32_t t=millis(), elapsed=t - g_micsT0;
  if (t - g_lastWarmMsgMs >= 1000) {
    uint32_t remain_ms = (elapsed >= MICS_WARMUP_MS) ? 0 : (MICS_WARMUP_MS - elapsed);
    Serial1.print(now_ms()); Serial1.print(", MICS_WARMUP, ");
    Serial1.println(remain_ms/1000);
    g_lastWarmMsgMs = t;
  }
  if (elapsed >= MICS_WARMUP_MS) {
    g_micsWarming=false; g_micsReady=true;
    Serial1.print(now_ms()); Serial1.println(", MICS_WARMUP_COMPLETE");
  }
}
static bool micsRead(float &co,float &nh3,float &h2,float &etoh,float &ch4){
  if (!g_micsPresent || !g_micsReady) return false;
  co   = max(0.0f, g_mics.getGasData(CO));
  nh3  = max(0.0f, g_mics.getGasData(NH3));
  h2   = max(0.0f, g_mics.getGasData(H2));
  etoh = max(0.0f, g_mics.getGasData(C2H5OH));
  ch4  = max(0.0f, g_mics.getGasData(CH4));
  return true;
}

// ============================================================================
// SHT21 (0x40)  -> Temp °C, RH %
// ============================================================================
#define SHT21_ADDR 0x40
#define SHT21_TRIG_TEMP_NOHOLD 0xF3
#define SHT21_TRIG_RH_NOHOLD   0xF5
#define SHT21_SOFT_RESET       0xFE
static int8_t g_shtCh=-1; static uint8_t g_shtAddr=0;
static uint8_t sht_crc8(const uint8_t *d,int n){ uint8_t c=0; for(int j=0;j<n;j++){ c^=d[j]; for(int i=0;i<8;i++) c=(c&0x80)?(uint8_t)((c<<1)^0x31):(uint8_t)(c<<1);} return c; }
static bool findSHT21(){ for(uint8_t ch=0; ch<8; ch++){ muxSelect(ch); if(i2cProbe(SHT21_ADDR)){ g_shtCh=ch; g_shtAddr=SHT21_ADDR; return true; } } return false; }
static bool sht21Write(uint8_t cmd){ muxSelect(g_shtCh); Wire.beginTransmission(g_shtAddr); Wire.write(cmd); return (Wire.endTransmission()==0); }
static bool sht21Init(){ muxSelect(g_shtCh); if(!sht21Write(SHT21_SOFT_RESET)) return false; delay(20); return true; }
static bool sht21Read(float &tC,float &rh){
  muxSelect(g_shtCh);
  if(!sht21Write(SHT21_TRIG_TEMP_NOHOLD)) return false; delay(85);
  if(Wire.requestFrom((int)g_shtAddr,3)!=3) return false;
  uint8_t tb[3]; tb[0]=Wire.read(); tb[1]=Wire.read(); tb[2]=Wire.read();
  if(sht_crc8(tb,2)!=tb[2]) return false;
  uint16_t traw=((uint16_t)tb[0]<<8)|(tb[1]&0xFC);
  tC = -46.85f + 175.72f * (float)traw / 65536.0f;

  if(!sht21Write(SHT21_TRIG_RH_NOHOLD)) return false; delay(30);
  if(Wire.requestFrom((int)g_shtAddr,3)!=3) return false;
  uint8_t hb[3]; hb[0]=Wire.read(); hb[1]=Wire.read(); hb[2]=Wire.read();
  if(sht_crc8(hb,2)!=hb[2]) return false;
  uint16_t hraw=((uint16_t)hb[0]<<8)|(hb[1]&0xFC);
  rh = -6.0f + 125.0f * (float)hraw / 65536.0f;
  rh = constrain(rh, 0.0f, 100.0f);
  return true;
}

// ============================================================================
// TSL2561 (0x29/0x39/0x49)  -> Lux, CH0, CH1   [ID check]
// ============================================================================
#define TSL2561_CMD         0x80
#define TSL2561_REG_CONTROL 0x00
#define TSL2561_REG_TIMING  0x01
#define TSL2561_REG_ID      0x0A
#define TSL2561_REG_CH0LOW  0x0C
#define TSL2561_REG_CH1LOW  0x0E
static const uint8_t kTSLAddrs[] = {0x29,0x39,0x49};
static int8_t g_tslCh=-1; static uint8_t g_tslAddr=0;
static bool tslWrite(uint8_t reg,uint8_t val){ muxSelect(g_tslCh); Wire.beginTransmission(g_tslAddr); Wire.write(TSL2561_CMD|reg); Wire.write(val); return (Wire.endTransmission()==0); }
static bool tslReadWord(uint8_t reg,uint16_t &out){ muxSelect(g_tslCh); Wire.beginTransmission(g_tslAddr); Wire.write(TSL2561_CMD|reg); if(Wire.endTransmission(false)!=0) return false; if(Wire.requestFrom((int)g_tslAddr,2)!=2) return false; uint8_t lo=Wire.read(),hi=Wire.read(); out=(hi<<8)|lo; return true; }
static bool tslReadByteAt(uint8_t addr,uint8_t reg,uint8_t &val){ Wire.beginTransmission(addr); Wire.write(TSL2561_CMD|reg); if(Wire.endTransmission(false)!=0) return false; if(Wire.requestFrom((int)addr,1)!=1) return false; val=Wire.read(); return true; }
static bool findTSL2561(){
  for(uint8_t ch=0; ch<8; ch++){
    muxSelect(ch);
    for(uint8_t i=0;i<sizeof(kTSLAddrs);i++){
      uint8_t addr=kTSLAddrs[i]; if(!i2cProbe(addr)) continue;
      uint8_t id=0; if(!tslReadByteAt(addr,TSL2561_REG_ID,id)) continue;
      if( ((id & 0xF0)==0x10) || (id==0x50) ) { g_tslCh=ch; g_tslAddr=addr; return true; }
    }
  }
  return false;
}
static bool tslInit(){ if(!tslWrite(TSL2561_REG_CONTROL,0x03)) return false; if(!tslWrite(TSL2561_REG_TIMING,0x02)) return false; delay(405); return true; }
static bool tslRead(float &lux,uint16_t &ch0,uint16_t &ch1){
  uint16_t b=0, ir=0; if(!tslReadWord(TSL2561_REG_CH0LOW,b)) return false; if(!tslReadWord(TSL2561_REG_CH1LOW,ir)) return false;
  ch0=b; ch1=ir; if(b==0){ lux=0; return true; }
  float ratio=(float)ir/(float)b;
  if      (ratio<=0.50f) lux=0.0304f*b - 0.062f*b*powf(ratio,1.4f);
  else if (ratio<=0.61f) lux=0.0224f*b - 0.031f*ir;
  else if (ratio<=0.80f) lux=0.0128f*b - 0.0153f*ir;
  else if (ratio<=1.30f) lux=0.00146f*b - 0.00112f*ir;
  else lux=0.0f;
  if(lux<0) lux=0;
  return true;
}

// ============================================================================
// AS726X (0x49) — Calibrated R..W mapped to A..F
// ============================================================================
#if __has_include(<SparkFun_AS726X.h>)
  #include <SparkFun_AS726X.h>
#else
  #include "AS726X.h"
#endif
AS726X g_as726x;
static int8_t g_asCh=-1; static const uint8_t g_asAddr=0x49;
static bool findAS726X(){ for(uint8_t ch=0; ch<8; ch++){ muxSelect(ch); if(i2cProbe(g_asAddr)){ g_asCh=ch; return true; } } return false; }
static bool as726xInit(){ muxSelect(g_asCh); if(!g_as726x.begin(Wire)) return false; g_as726x.setGain(3); g_as726x.setIntegrationTime(50); g_as726x.setMeasurementMode(3); return true; }
static bool as726xRead(float &A,float &B,float &C,float &D,float &E,float &F){ muxSelect(g_asCh); float R=g_as726x.getCalibratedR(),S=g_as726x.getCalibratedS(),T=g_as726x.getCalibratedT(),U=g_as726x.getCalibratedU(),V=g_as726x.getCalibratedV(),W=g_as726x.getCalibratedW(); A=R;B=S;C=T;D=U;E=V;F=W; return true; }

// ============================================================================
// MPL115A2 (0x60) — Pressure kPa, Temp est °C
// ============================================================================
#define MPL115A2_ADDR 0x60
#define MPL115A2_REG_A0_MSB         0x04
#define MPL115A2_REG_PRESSURE_MSB   0x00
#define MPL115A2_REG_START_CONVERT  0x12
static int8_t g_mplCh=-1; static uint8_t g_mplAddr=0;
static float c_a0=0,c_b1=0,c_b2=0,c_c12=0;
static bool mplReadN(uint8_t reg,uint8_t *buf,size_t n){ muxSelect(g_mplCh); Wire.beginTransmission(g_mplAddr); Wire.write(reg); if(Wire.endTransmission(false)!=0) return false; if(Wire.requestFrom((int)g_mplAddr,(int)n)!=(int)n) return false; for(size_t i=0;i<n;i++) buf[i]=Wire.read(); return true; }
static bool mplWrite(uint8_t reg,uint8_t val){ muxSelect(g_mplCh); Wire.beginTransmission(g_mplAddr); Wire.write(reg); Wire.write(val); return (Wire.endTransmission()==0); }
static bool findMPL115A2(){ for(uint8_t ch=0; ch<8; ch++){ muxSelect(ch); if(i2cProbe(MPL115A2_ADDR)){ g_mplCh=ch; g_mplAddr=MPL115A2_ADDR; return true; } } return false; }
static bool mplInit(){ uint8_t c[8]; if(!mplReadN(MPL115A2_REG_A0_MSB,c,8)) return false; int16_t a0=(int16_t)((c[0]<<8)|c[1]); int16_t b1=(int16_t)((c[2]<<8)|c[3]); int16_t b2=(int16_t)((c[4]<<8)|c[5]); int16_t c12=(int16_t)((c[6]<<8)|c[7]); c_c12=(float)(c12>>2)/4194304.0f; c_a0=(float)a0/8.0f; c_b1=(float)b1/8192.0f; c_b2=(float)b2/16384.0f; return true; }
static bool mplRead(float &p_kPa,float &t_C){ if(!mplWrite(MPL115A2_REG_START_CONVERT,0x00)) return false; delay(5); uint8_t r[4]; if(!mplReadN(MPL115A2_REG_PRESSURE_MSB,r,4)) return false; uint16_t Padc=((uint16_t)r[0]<<8|r[1])>>6; uint16_t Tadc=((uint16_t)r[2]<<8|r[3])>>6; float Pcomp=c_a0+(c_b1+c_c12*(float)Tadc)*(float)Padc + c_b2*(float)Tadc; p_kPa=Pcomp*(65.0f/1023.0f)+50.0f; t_C=(float)Tadc*(-0.1736f)+111.5f; return true; }

// ============================================================================
// Aggregation + CSV header
// ============================================================================
static bool headerPrinted=false;
static uint32_t lastLogMs=0;

// Last-read values + flags
static bool SHT_FLAG=false, TSL_FLAG=false, AS_FLAG=false, MPL_FLAG=false, MICS_FLAG=false;
static float SHT_T=0, SHT_RH=0;
static float TSL_lux=0; static uint16_t TSL_ch0=0, TSL_ch1=0;
static float AS_A=0,AS_B=0,AS_C=0,AS_D=0,AS_E=0,AS_F=0;
static float MPL_P=0, MPL_T=0;
static float GUVA_mV=0;
static float MICS_CO=0,MICS_NH3=0,MICS_H2=0,MICS_ETOH=0,MICS_CH4=0;

// Print single CSV header (one time)
static void printHeaderOnce() {
  if (headerPrinted) return;
  Serial1.println(
    "timestamp_ms,"
    "SHT_FLAG,SHT_ch,SHT_addr_hex,SHT_T_C,SHT_RH_pct,"
    "TSL_FLAG,TSL_ch,TSL_addr_hex,TSL_lux,TSL_CH0,TSL_CH1,"
    "AS_FLAG,AS_ch,AS_addr_hex,AS_A,AS_B,AS_C,AS_D,AS_E,AS_F,"
    "MPL_FLAG,MPL_ch,MPL_addr_hex,MPL_P_kPa,MPL_T_C_est,"
    "GUVA_mV,"
    "MICS_FLAG,MICS_CO_ppm,MICS_NH3_ppm,MICS_H2_ppm,MICS_EtOH_ppm,MICS_CH4_ppm"
  );
  headerPrinted = true;
}

// ============================================================================
// SETUP / LOOP
// ============================================================================
void setup() {
  Serial1.begin(115200, SERIAL_8E1); delay(100);

  Wire.setSDA(PD13); Wire.setSCL(PD12); Wire.begin(); Wire.setClock(400000);

  // Quick device list
  Serial1.println("INFO,ScanStart");
  for(uint8_t ch=0; ch<8; ch++){
    muxSelect(ch);
    bool any=false;
    for(uint8_t a=3;a<0x78;a++){ if(a==MUX_ADDR) continue; if(i2cProbe(a)){ if(!any){Serial1.print("Ch ");Serial1.print(ch);Serial1.print(": "); any=true;} Serial1.print("0x"); if(a<16)Serial1.print('0'); Serial1.print(a,HEX); Serial1.print(' ');} }
    if(any) Serial1.println();
  }
  Serial1.println("INFO,ScanEnd");

  // Init analog + MiCS (non-blocking warmup)
  guvaInit();
  micsInitNonBlocking();

  // I2C find + init
  if(findSHT21() && sht21Init()){ Serial1.print("INFO,SHT21,ch,"); Serial1.print(g_shtCh); Serial1.print(",addr,0x"); Serial1.println(g_shtAddr,HEX); }
  else Serial1.println("WARN,SHT21,not_found");

  if(findTSL2561() && tslInit()){ Serial1.print("INFO,TSL2561,ch,"); Serial1.print(g_tslCh); Serial1.print(",addr,0x"); Serial1.println(g_tslAddr,HEX); }
  else Serial1.println("WARN,TSL2561,not_found_or_bad_id");

  if(findAS726X() && as726xInit()){ Serial1.print("INFO,AS726X,ch,"); Serial1.print(g_asCh); Serial1.println(",addr,0x49"); }
  else Serial1.println("WARN,AS726X,not_found_or_init_fail");

  if(findMPL115A2() && mplInit()){ Serial1.print("INFO,MPL115A2,ch,"); Serial1.print(g_mplCh); Serial1.print(",addr,0x"); Serial1.println(g_mplAddr,HEX); }
  else Serial1.println("WARN,MPL115A2,not_found");

  printHeaderOnce();
  lastLogMs = millis();
}

void loop() {
  const uint32_t t = now_ms();

  // Service non-blocking MiCS warmup (prints 1 line/sec countdown)
  micsServiceWarmup();

  // ---- fast poll (update cached values) ----
  // SHT21
  if (g_shtCh >= 0) { float tc=0,rh=0; if (sht21Read(tc,rh)) { SHT_FLAG=true; SHT_T=tc; SHT_RH=rh; } else SHT_FLAG=false; }

  // TSL2561
  if (g_tslCh >= 0) { float lux=0; uint16_t ch0=0,ch1=0; if (tslRead(lux,ch0,ch1)) { TSL_FLAG=true; TSL_lux=lux; TSL_ch0=ch0; TSL_ch1=ch1; } else TSL_FLAG=false; }

  // AS726X
  if (g_asCh >= 0) { float A,B,C,D,E,F; if (as726xRead(A,B,C,D,E,F)) { AS_FLAG=true; AS_A=A;AS_B=B;AS_C=C;AS_D=D;AS_E=E;AS_F=F; } else AS_FLAG=false; }

  // MPL115A2
  if (g_mplCh >= 0) { float p=0,te=0; if (mplRead(p,te)) { MPL_FLAG=true; MPL_P=p; MPL_T=te; } else MPL_FLAG=false; }

  // GUVA (always present if pin wired)
  { float mv=0; if (guvaRead(mv)) GUVA_mV=mv; }

  // MiCS (only after warmup)
  if (g_micsReady) { float co,nh3,h2,etoh,ch4; if (micsRead(co,nh3,h2,etoh,ch4)) { MICS_FLAG=true; MICS_CO=co; MICS_NH3=nh3; MICS_H2=h2; MICS_ETOH=etoh; MICS_CH4=ch4; } else MICS_FLAG=false; }
  else MICS_FLAG=false;

  // ---- emit one CSV row every LOG_INTERVAL_MS ----
  if (t - lastLogMs >= LOG_INTERVAL_MS) {
    lastLogMs = t;

    Serial1.print(t); Serial1.print(',');

    // SHT21
    Serial1.print(SHT_FLAG?1:0); Serial1.print(',');
    Serial1.print(g_shtCh); Serial1.print(','); Serial1.print("0x"); Serial1.print(g_shtAddr,HEX); Serial1.print(',');
    Serial1.print(SHT_T,2); Serial1.print(','); Serial1.print(SHT_RH,1); Serial1.print(',');

    // TSL2561
    Serial1.print(TSL_FLAG?1:0); Serial1.print(',');
    Serial1.print(g_tslCh); Serial1.print(','); Serial1.print("0x"); Serial1.print(g_tslAddr,HEX); Serial1.print(',');
    Serial1.print(TSL_lux,1); Serial1.print(','); Serial1.print(TSL_ch0); Serial1.print(','); Serial1.print(TSL_ch1); Serial1.print(',');

    // AS726X
    Serial1.print(AS_FLAG?1:0); Serial1.print(',');
    Serial1.print(g_asCh); Serial1.print(','); Serial1.print("0x"); Serial1.print(g_asAddr,HEX); Serial1.print(',');
    Serial1.print(AS_A,3); Serial1.print(','); Serial1.print(AS_B,3); Serial1.print(',');
    Serial1.print(AS_C,3); Serial1.print(','); Serial1.print(AS_D,3); Serial1.print(',');
    Serial1.print(AS_E,3); Serial1.print(','); Serial1.print(AS_F,3); Serial1.print(',');

    // MPL115A2
    Serial1.print(MPL_FLAG?1:0); Serial1.print(',');
    Serial1.print(g_mplCh); Serial1.print(','); Serial1.print("0x"); Serial1.print(g_mplAddr,HEX); Serial1.print(',');
    Serial1.print(MPL_P,3); Serial1.print(','); Serial1.print(MPL_T,2); Serial1.print(',');

    // GUVA
    Serial1.print(GUVA_mV,1); Serial1.print(',');

    // MiCS
    Serial1.print(MICS_FLAG?1:0); Serial1.print(',');
    Serial1.print(MICS_CO,2);   Serial1.print(',');
    Serial1.print(MICS_NH3,2);  Serial1.print(',');
    Serial1.print(MICS_H2,2);   Serial1.print(',');
    Serial1.print(MICS_ETOH,2); Serial1.print(',');
    Serial1.println(MICS_CH4,2);
  }

  // pacing for sensor polls
  delay(120);
}
