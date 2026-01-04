// ===== GPS (UART3) + SD CSV Logger (STM32 custom board) =====
// Output: Serial1 (115200, 8E1)  |  GPS: Serial3 (9600, 8N1)
// SDIO remap pins per your sample (PC8..PC12 + PD2), detect PB8
// File: /gpslog.csv  (appends; writes header if file is new)

#include <Arduino.h>
#include <STM32SD.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

// ---- UARTs ----
HardwareSerial Serial1(PA10, PA9);   // Bluetooth / output
HardwareSerial Serial3(PD9,  PD8);   // GPS

// ---- SD mapping ----
#ifndef SD_DETECT_PIN
#define SD_DETECT_PIN PB8
#endif

// SDIO pin remap (from your sample)
static void setupSDPins() {
  SD.setDx(PC8, PC9, PC10, PC11);
  SD.setCMD(PD2);
  SD.setCK(PC12);
}

File SensorData;
static const char* LOG_NAME = "gpslog.csv";

// ---- Tunables ----
static const int   TZ_OFFSET_MIN      = +240;  // Dubai UTC+4
static const float PRINT_PERIOD_MS    = 1000.0f;

// ---- GPS parse buffers/state ----
static char  lineBuf[128];  static int lineLen = 0;
static bool  haveRMC=false, haveGGA=false;

static float latDeg=NAN, lonDeg=NAN;         // latest parsed (decimal degrees)
static float sogKn=NAN, cogDeg=NAN;          // speed(knots), course(deg)
static int   fixQ=0, sats=0;                 // GGA fix quality & satellites used
static float hdop=NAN, altMSL=NAN, geoidSep=NAN; // meters

// UTC date/time (from RMC)
static int utc_h=-1, utc_m=-1, utc_s=-1;
static int utc_d=-1, utc_M=-1, utc_y=-1;     // yy (00..99)

static uint32_t lastTick = 0;

// ---- Helpers ----
static float nmeaToDeg(const char* s, char hemi) {
  if (!s || !*s) return NAN;
  float v = atof(s);
  int deg = (int)(v / 100.0f);
  float min = v - (deg * 100.0f);
  float out = deg + (min / 60.0f);
  if (hemi=='S' || hemi=='W') out = -out;
  return out;
}

static void parseUTC(const char* tHHMMSS, const char* dDDMMYY) {
  if (tHHMMSS && strlen(tHHMMSS) >= 6) {
    utc_h = (tHHMMSS[0]-'0')*10 + (tHHMMSS[1]-'0');
    utc_m = (tHHMMSS[2]-'0')*10 + (tHHMMSS[3]-'0');
    utc_s = (tHHMMSS[4]-'0')*10 + (tHHMMSS[5]-'0');
  }
  if (dDDMMYY && strlen(dDDMMYY) >= 6) {
    utc_d = (dDDMMYY[0]-'0')*10 + (dDDMMYY[1]-'0');
    utc_M = (dDDMMYY[2]-'0')*10 + (dDDMMYY[3]-'0');
    utc_y = (dDDMMYY[4]-'0')*10 + (dDDMMYY[5]-'0');
  }
}
static bool isLeap(int y){ return ((y%4==0)&&(y%100!=0)) || (y%400==0); }
static int  dim(int y,int m){ static const int d[12]={31,28,31,30,31,30,31,31,30,31,30,31}; return m==2? d[m-1]+(isLeap(y)?1:0):d[m-1]; }

static void makeUtcStrings(char* dateBuf, size_t dsz, char* timeBuf, size_t tsz) {
  if (utc_d<0) { if(dsz) dateBuf[0]='\0'; if(tsz) timeBuf[0]='\0'; return; }
  int Y = 2000 + utc_y;
  snprintf(dateBuf, dsz, "%04d-%02d-%02d", Y, utc_M, utc_d);
  snprintf(timeBuf, tsz, "%02d:%02d:%02d", utc_h, utc_m, utc_s);
}
static void makeLocalStrings(char* dateBuf, size_t dsz, char* timeBuf, size_t tsz) {
  if (utc_d<0) { if(dsz) dateBuf[0]='\0'; if(tsz) timeBuf[0]='\0'; return; }
  int Y = 2000 + utc_y, M = utc_M, D = utc_d;
  int totalMin = utc_h*60 + utc_m + TZ_OFFSET_MIN;
  if (totalMin < 0) { totalMin += 1440; D -= 1; }
  if (totalMin >= 1440) { totalMin -= 1440; D += 1; }
  int H = totalMin / 60, Min = totalMin % 60, S = utc_s;
  if (D <= 0) { M--; if (M<=0){M=12; Y--; } D = dim(Y,M); }
  int md = dim(Y,M); if (D > md){ D=1; M++; if(M>12){M=1; Y++;}}
  snprintf(dateBuf, dsz, "%04d-%02d-%02d", Y, M, D);
  snprintf(timeBuf, tsz, "%02d:%02d:%02d", H, Min, S);
}

// ---- Parsers ----
static void handleRMC(char* s) {
  // $GxRMC,1:time,2:status(A/V),3:lat,4:N/S,5:lon,6:E/W,7:sog(kn),8:cog(deg),9:date,...
  char* tok[13] = {0};
  int n=0; char* p=strtok(s,","); while(p && n<13){ tok[n++]=p; p=strtok(NULL,","); }
  if (n < 10) return;
  bool valid = (tok[2] && tok[2][0]=='A');
  if (tok[1] && tok[9]) parseUTC(tok[1], tok[9]);
  if (valid) {
    float la = nmeaToDeg(tok[3], tok[4]?tok[4][0]:'N');
    float lo = nmeaToDeg(tok[5], tok[6]?tok[6][0]:'E');
    if (!isnan(la) && !isnan(lo)) { latDeg = la; lonDeg = lo; }
    sogKn  = (tok[7] && *tok[7]) ? atof(tok[7]) : NAN;
    cogDeg = (tok[8] && *tok[8]) ? atof(tok[8]) : NAN;
    haveRMC = true;
  }
}
static void handleGGA(char* s) {
  // $GxGGA,1:time,2:lat,3:N/S,4:lon,5:E/W,6:fixQ,7:sats,8:HDOP,9:altMSL,10:M,11:geoid,12:M,...
  char* tok[16] = {0};
  int n=0; char* p=strtok(s,","); while(p && n<16){ tok[n++]=p; p=strtok(NULL,","); }
  if (n < 10) return;
  fixQ = (tok[6] && *tok[6]) ? atoi(tok[6]) : 0;
  sats = (tok[7] && *tok[7]) ? atoi(tok[7]) : 0;
  hdop = (tok[8] && *tok[8]) ? atof(tok[8]) : NAN;
  altMSL   = (tok[9]  && *tok[9])  ? atof(tok[9])  : NAN;
  geoidSep = (tok[11] && *tok[11]) ? atof(tok[11]) : NAN;

  if ((isnan(latDeg) || isnan(lonDeg)) && tok[2] && tok[3] && tok[4] && tok[5]) {
    float la = nmeaToDeg(tok[2], tok[3][0]);
    float lo = nmeaToDeg(tok[4], tok[5][0]);
    if (!isnan(la) && !isnan(lo)) { latDeg = la; lonDeg = lo; }
  }
  haveGGA = true;
}

// ---- CSV logging ----
static void writeHeaderIfNew(File &f) {
  if (f && f.size() == 0) {
    f.println("utc_date,utc_time,local_date,local_time,lat_deg,lon_deg,alt_msl_m,alt_wgs84_m,geoid_sep_m,speed_kn,course_deg,fixQ,sats,hdop");
    f.flush();
  }
}
static void writeCSVRow(File &f) {
  if (!f) return;

  char utcDate[16]={0}, utcTime[16]={0}, locDate[16]={0}, locTime[16]={0};
  makeUtcStrings(utcDate, sizeof(utcDate), utcTime, sizeof(utcTime));
  makeLocalStrings(locDate, sizeof(locDate), locTime, sizeof(locTime));

  auto printOrBlank = [&](float v, int prec) {
    if (isnan(v)) f.print(""); else f.print(String(v, prec));
  };

  // utc_date,utc_time,local_date,local_time
  f.print(utcDate); f.print(",");
  f.print(utcTime); f.print(",");
  f.print(locDate); f.print(",");
  f.print(locTime); f.print(",");

  // lat_deg,lon_deg
  printOrBlank(latDeg, 6); f.print(",");
  printOrBlank(lonDeg, 6); f.print(",");

  // alt_msl_m, alt_wgs84_m, geoid_sep_m
  printOrBlank(altMSL, 1); f.print(",");
  if (!isnan(altMSL) && !isnan(geoidSep)) f.print(String(altMSL + geoidSep, 1)); else f.print("");
  f.print(",");
  printOrBlank(geoidSep, 1); f.print(",");

  // speed_kn, course_deg
  printOrBlank(sogKn, 2); f.print(",");
  printOrBlank(cogDeg, 2); f.print(",");

  // fixQ, sats, hdop
  f.print(fixQ); f.print(",");
  f.print(sats); f.print(",");
  printOrBlank(hdop, 2);

  f.println();
  f.flush(); // mission: flush each second for safety
}

// ---- Status console (optional, human-readable) ----
static void printStatus() {
  Serial1.println("========== GPS STATUS ==========");
  Serial1.print("[GPS] FixQ: "); Serial1.print(fixQ);
  Serial1.print(" | Sats: "); Serial1.print(sats);
  Serial1.print(" | HDOP: "); if (isnan(hdop)) Serial1.print("-"); else Serial1.print(hdop, 2);
  Serial1.println();

  Serial1.print("[GPS] Pos : ");
  if (!isnan(latDeg) && !isnan(lonDeg)) {
    Serial1.print(latDeg, 6); Serial1.print(", "); Serial1.println(lonDeg, 6);
  } else Serial1.println("-");

  Serial1.print("[GPS] Alt : ");
  if (isnan(altMSL)) Serial1.println("-");
  else {
    Serial1.print(altMSL, 1); Serial1.print(" m MSL");
    if (!isnan(geoidSep)) {
      Serial1.print("  |  WGS84: "); Serial1.print(altMSL + geoidSep, 1); Serial1.print(" m");
      Serial1.print("  (geoid "); Serial1.print(geoidSep, 1); Serial1.print(" m)");
    }
    Serial1.println();
  }

  Serial1.print("[GPS] Move: Speed(kn) ");
  if (isnan(sogKn)) Serial1.print("-"); else Serial1.print(sogKn, 2);
  Serial1.print(" | Course(deg) ");
  if (isnan(cogDeg)) Serial1.println("-"); else Serial1.println(cogDeg, 1);

  char uD[16], uT[16], lD[16], lT[16];
  makeUtcStrings(uD,sizeof(uD),uT,sizeof(uT));
  makeLocalStrings(lD,sizeof(lD),lT,sizeof(lT));
  Serial1.print("[GPS] UTC  : "); Serial1.print(uT); Serial1.print("  "); Serial1.println(uD);
  Serial1.print("[GPS] Dubai: "); Serial1.print(lT); Serial1.print("  "); Serial1.println(lD);
  Serial1.println("================================\n");
}

// ---- Setup / Loop ----
void setup() {
  Serial1.begin(115200, SERIAL_8E1);
  Serial3.begin(9600);

  setupSDPins();
  Serial1.print("SD init...");
  while (!SD.begin()); // wait for card
  Serial1.println("OK");

  SensorData = SD.open(LOG_NAME, FILE_WRITE);
  if (!SensorData) {
    Serial1.println("ERROR: Cannot open gpslog.csv");
    while (1) { delay(1000); }
  }
  writeHeaderIfNew(SensorData);

  Serial1.println("=== GPS + SD Logger Ready ===");
  Serial1.println("Logging 1 Hz to /gpslog.csv");
  Serial1.println("================================\n");
}

void loop() {
  while (Serial3.available()) {
    char c = (char)Serial3.read();
    if (c == '\r') continue;
    if (c == '\n') {
      lineBuf[lineLen] = '\0';
      if (lineLen > 6 && lineBuf[0] == '$') {
        char work[128]; strncpy(work, lineBuf, sizeof(work)); work[sizeof(work)-1] = '\0';
        if (!strncmp(work,"$GPRMC",6) || !strncmp(work,"$GNRMC",6))      handleRMC(work);
        else if (!strncmp(work,"$GPGGA",6) || !strncmp(work,"$GNGGA",6))  handleGGA(work);
      }
      lineLen = 0;
    } else {
      if (lineLen < (int)sizeof(lineBuf)-1) lineBuf[lineLen++] = c;
      else lineLen = 0; // overflow guard
    }
  }

  // Once per second: print status + log CSV
  uint32_t now = millis();
  if (now - lastTick >= PRINT_PERIOD_MS) {
    printStatus();
    writeCSVRow(SensorData);
    lastTick = now;
  }
}
