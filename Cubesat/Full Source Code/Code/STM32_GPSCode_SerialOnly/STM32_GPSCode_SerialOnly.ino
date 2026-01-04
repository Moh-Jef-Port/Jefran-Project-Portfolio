// // // // // NEO GPS (UART3) -> Bluetooth (UART1) passthrough + LAT/LON parser
// // // // // Board style preserved: IntroStratLib, Serial1(PA10,PA9), Serial3(PD9,PD8)

// // // // #include <Arduino.h>

// // // // HardwareSerial Serial1(PA10, PA9);  // Bluetooth (115200, 8E1)
// // // // HardwareSerial Serial3(PD9,  PD8);  // GPS (9600, 8N1)  RX3=PD9, TX3=PD8

// // // // // --- Simple NMEA helpers ---
// // // // static bool nmeaChecksumOK(const char* s) {
// // // //   // expects string that starts with '$' and contains '*XX'
// // // //   if (!s || s[0] != '$') return false;
// // // //   const char* star = nullptr;
// // // //   uint8_t chk = 0;
// // // //   for (const char* p = s + 1; *p; ++p) {
// // // //     if (*p == '*') { star = p; break; }
// // // //     chk ^= (uint8_t)(*p);
// // // //   }
// // // //   if (!star || !star[1] || !star[2]) return false;
// // // //   auto hex = [](char c)->int {
// // // //     if (c >= '0' && c <= '9') return c - '0';
// // // //     if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
// // // //     if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
// // // //     return -1;
// // // //   };
// // // //   int hi = hex(star[1]), lo = hex(star[2]);
// // // //   if (hi < 0 || lo < 0) return false;
// // // //   uint8_t want = (uint8_t)((hi << 4) | lo);
// // // //   return chk == want;
// // // // }

// // // // static double nmeaToDegrees(const char* ddmm) {
// // // //   // Converts ddmm.mmmm (lat) or dddmm.mmmm (lon) to decimal degrees
// // // //   if (!ddmm || !*ddmm) return NAN;
// // // //   // find decimal point to split minutes
// // // //   const char* dot = strchr(ddmm, '.');
// // // //   int len = dot ? (int)(dot - ddmm) : (int)strlen(ddmm);
// // // //   if (len < 3) return NAN; // need at least dmm
// // // //   int degDigits = (len == 4 || len == 5) ? (len - 2) : (len - 2); // general case
// // // //   // latitude: 2 deg digits; longitude: 3 deg digits — this formula handles both
// // // //   char dgbuf[4] = {0,0,0,0};
// // // //   strncpy(dgbuf, ddmm, degDigits);
// // // //   int deg = atoi(dgbuf);
// // // //   double minutes = atof(ddmm + degDigits);
// // // //   return (double)deg + (minutes / 60.0);
// // // // }

// // // // static bool parseRMC(char* line, double& lat, double& lon, bool& valid) {
// // // //   // $GxRMC,hhmmss.sss,A,llll.ll,a,yyyyy.yy,a,sog,cog,ddmmyy,mag,magE/W,mode*CS
// // // //   // tokens: 0=$GxRMC 1=time 2=status 3=lat 4=N/S 5=lon 6=E/W ...
// // // //   const int MAXF = 20;
// // // //   char* f[MAXF] = {0};
// // // //   int n = 0;
// // // //   for (char* p = line; *p && n < MAXF; ++n, p = nullptr) f[n] = strtok(p, ",");
// // // //   if (n < 7) return false;
// // // //   valid = (f[2] && f[2][0] == 'A');
// // // //   if (!valid) return true; // sentence is fine but no fix
// // // //   if (!f[3] || !f[4] || !f[5] || !f[6]) return false;

// // // //   double la = nmeaToDegrees(f[3]);
// // // //   double lo = nmeaToDegrees(f[5]);
// // // //   if (isnan(la) || isnan(lo)) return false;
// // // //   if (f[4][0] == 'S') la = -la;
// // // //   if (f[6][0] == 'W') lo = -lo;
// // // //   lat = la; lon = lo;
// // // //   return true;
// // // // }

// // // // static bool parseGGA(char* line, double& lat, double& lon, bool& valid) {
// // // //   // $GxGGA,hhmmss.sss,lat,N/S,lon,E/W,fix,numsats,HDOP,alt,M,...
// // // //   const int MAXF = 20;
// // // //   char* f[MAXF] = {0};
// // // //   int n = 0;
// // // //   for (char* p = line; *p && n < MAXF; ++n, p = nullptr) f[n] = strtok(p, ",");
// // // //   if (n < 7) return false;
// // // //   int fix = f[6] ? atoi(f[6]) : 0;
// // // //   valid = fix > 0;
// // // //   if (!f[2] || !f[3] || !f[4] || !f[5]) return false;

// // // //   double la = nmeaToDegrees(f[2]);
// // // //   double lo = nmeaToDegrees(f[4]);
// // // //   if (isnan(la) || isnan(lo)) return false;
// // // //   if (f[3][0] == 'S') la = -la;
// // // //   if (f[5][0] == 'W') lo = -lo;
// // // //   lat = la; lon = lo;
// // // //   return true;
// // // // }

// // // // // --- State ---
// // // // static char lineBuf[128];
// // // // static int  lineLen = 0;

// // // // void setup() {
// // // //   Serial1.begin(115200, SERIAL_8E1);
// // // //   Serial3.begin(9600); // NEO default

// // // //   Serial1.println("\nGPS passthrough + parser started");
// // // //   Serial1.println("UART3 <= NEO GPS @9600  ->  UART1 (BT) @115200");
// // // //   Serial1.println("Expect raw NMEA lines + parsed LAT,LON when fix is valid.\n");
// // // // }

// // // // void loop() {
// // // //   // 1) Passthrough raw GPS to Bluetooth AND accumulate lines for parsing
// // // //   while (Serial3.available()) {
// // // //     char c = (char)Serial3.read();
// // // //     Serial1.write(c);  // passthrough so you can see NMEA live

// // // //     if (c == '\r') continue;
// // // //     if (c == '\n') {
// // // //       lineBuf[lineLen] = '\0';
// // // //       if (lineLen > 6 && lineBuf[0] == '$') {
// // // //         // 2) Validate checksum
// // // //         if (nmeaChecksumOK(lineBuf)) {
// // // //           // Work on a copy because strtok modifies the buffer
// // // //           char work[128];
// // // //           strncpy(work, lineBuf, sizeof(work));
// // // //           work[sizeof(work)-1] = '\0';

// // // //           // Detect sentence type
// // // //           bool parsed = false, valid = false;
// // // //           double lat = NAN, lon = NAN;

// // // //           if (!strncmp(work, "$GPRMC", 6) || !strncmp(work, "$GNRMC", 6)) {
// // // //             parsed = parseRMC(work, lat, lon, valid);
// // // //           } else {
// // // //             // try GGA as well
// // // //             strncpy(work, lineBuf, sizeof(work)); work[sizeof(work)-1]='\0';
// // // //             if (!strncmp(work, "$GPGGA", 6) || !strncmp(work, "$GNGGA", 6)) {
// // // //               parsed = parseGGA(work, lat, lon, valid);
// // // //             }
// // // //           }

// // // //           if (parsed) {
// // // //             if (valid) {
// // // //               Serial1.print("PARSED: ");
// // // //               Serial1.print(lat, 6);
// // // //               Serial1.print(", ");
// // // //               Serial1.println(lon, 6);
// // // //             } else {
// // // //               Serial1.println("PARSED: no valid fix yet (status)");
// // // //             }
// // // //           }
// // // //         } else {
// // // //           Serial1.println("WARN: checksum fail");
// // // //         }
// // // //       }
// // // //       // reset buffer
// // // //       lineLen = 0;
// // // //     } else {
// // // //       if (lineLen < (int)sizeof(lineBuf)-1) {
// // // //         lineBuf[lineLen++] = c;
// // // //       } else {
// // // //         // overflow safeguard
// // // //         lineLen = 0;
// // // //       }
// // // //     }
// // // //   }

// // // //   // 3) (Optional) You could add a small delay; not necessary here
// // // //   // delay(1);
// // // // }

// // // // NEO GPS (UART3) -> Serial1 (Bluetooth) with friendly status lines
// // // // Hardware: NEO TX -> PD9 (RX3), NEO RX -> PD8 (TX3), 3V3 & GND

// // // #include <Arduino.h>

// // // HardwareSerial Serial1(PA10, PA9);  // Output / Bluetooth (115200, 8E1)
// // // HardwareSerial Serial3(PD9,  PD8);  // GPS (9600, 8N1)

// // // // Toggle this if you want to see raw NMEA lines echoed to Serial1 too
// // // static const bool SHOW_RAW_NMEA = true;

// // // static char  lineBuf[128];
// // // static int   lineLen = 0;

// // // static float lastLat = NAN, lastLon = NAN;
// // // static int   lastFix = -1, lastSats = -1;
// // // static float lastHdop = NAN, lastAlt = NAN;

// // // // Convert NMEA ddmm.mmmm / dddmm.mmmm + hemisphere to decimal degrees
// // // static float nmeaToDeg(const char* s, char hemi) {
// // //   if (!s || !*s) return NAN;
// // //   float v = atof(s);
// // //   int deg = (int)(v / 100.0f);
// // //   float min = v - (deg * 100.0f);
// // //   float out = deg + (min / 60.0f);
// // //   if (hemi == 'S' || hemi == 'W') out = -out;
// // //   return out;
// // // }

// // // static const char* ggaFixQualityName(int q) {
// // //   switch (q) {
// // //     case 0: return "No Fix";
// // //     case 1: return "GPS Fix";
// // //     case 2: return "DGPS Fix";
// // //     case 3: return "PPS Fix";
// // //     case 4: return "RTK Fixed";
// // //     case 5: return "RTK Float";
// // //     case 6: return "Dead Reckoning";
// // //     default: return "Unknown";
// // //   }
// // // }

// // // // Parse a $GxRMC sentence -> prints LAT/LON + speed/track if valid
// // // static void handleRMC(char* s) {
// // //   // $GxRMC,1:time,2:status(A/V),3:lat,4:N/S,5:lon,6:E/W,7:sog(kn),8:cog(deg),9:date,...
// // //   char* tok[13] = {0};
// // //   int n = 0; char* p = strtok(s, ",");
// // //   while (p && n < 13) { tok[n++] = p; p = strtok(NULL, ","); }
// // //   if (n < 7) return;

// // //   bool valid = (tok[2] && tok[2][0] == 'A');
// // //   if (!valid) {
// // //     Serial1.println("[RMC] Status: NO FIX (V)");
// // //     return;
// // //   }

// // //   float lat = nmeaToDeg(tok[3], tok[4] ? tok[4][0] : 'N');
// // //   float lon = nmeaToDeg(tok[5], tok[6] ? tok[6][0] : 'E');
// // //   float sog = (tok[7] && *tok[7]) ? atof(tok[7]) : NAN; // knots
// // //   float cog = (tok[8] && *tok[8]) ? atof(tok[8]) : NAN; // degrees

// // //   Serial1.print("[RMC] Position: ");
// // //   Serial1.print(lat, 6); Serial1.print(", ");
// // //   Serial1.print(lon, 6);
// // //   if (!isnan(sog)) { Serial1.print(" | Speed(kn): "); Serial1.print(sog, 2); }
// // //   if (!isnan(cog)) { Serial1.print(" | Course(deg): "); Serial1.print(cog, 2); }
// // //   Serial1.println();

// // //   lastLat = lat; lastLon = lon; // remember last position we printed
// // // }

// // // // Parse a $GxGGA sentence -> prints fix quality, sats, HDOP, altitude
// // // static void handleGGA(char* s) {
// // //   // $GxGGA,1:time,2:lat,3:N/S,4:lon,5:E/W,6:fixQ,7:sats,8:HDOP,9:alt,10:M,...
// // //   char* tok[15] = {0};
// // //   int n = 0; char* p = strtok(s, ",");
// // //   while (p && n < 15) { tok[n++] = p; p = strtok(NULL, ","); }
// // //   if (n < 10) return;

// // //   int   fixQ = (tok[6] && *tok[6]) ? atoi(tok[6]) : 0;
// // //   int   sats = (tok[7] && *tok[7]) ? atoi(tok[7]) : 0;
// // //   float hdop = (tok[8] && *tok[8]) ? atof(tok[8]) : NAN;
// // //   float alt  = (tok[9] && *tok[9]) ? atof(tok[9]) : NAN;

// // //   // Only print when something changes or on first run
// // //   bool changed = (fixQ != lastFix) || (sats != lastSats) ||
// // //                  (isnan(hdop) != isnan(lastHdop)) || (isnan(alt) != isnan(lastAlt)) ||
// // //                  (!isnan(hdop) && fabsf(hdop - lastHdop) > 0.1f) ||
// // //                  (!isnan(alt)  && fabsf(alt  - lastAlt)  > 0.5f);

// // //   if (changed) {
// // //     Serial1.print("[GGA] Fix: ");
// // //     Serial1.print(ggaFixQualityName(fixQ));
// // //     Serial1.print(" ("); Serial1.print(fixQ); Serial1.print(")");
// // //     Serial1.print(" | Sats: "); Serial1.print(sats);
// // //     Serial1.print(" | HDOP: "); if (isnan(hdop)) Serial1.print("-"); else Serial1.print(hdop, 2);
// // //     Serial1.print(" | Alt(m): "); if (isnan(alt)) Serial1.print("-"); else Serial1.print(alt, 1);
// // //     Serial1.println();

// // //     lastFix = fixQ; lastSats = sats; lastHdop = hdop; lastAlt = alt;
// // //   }
// // // }

// // // void setup() {
// // //   Serial1.begin(115200, SERIAL_8E1);
// // //   Serial3.begin(9600);

// // //   Serial1.println();
// // //   Serial1.println("=== GPS Reader Started ===");
// // //   Serial1.println("UART3 <= NEO GPS @9600  ->  UART1 (BT) @115200 8E1");
// // //   Serial1.println("Tips: Go outdoors, antenna face-up; first cold fix can take minutes.");
// // //   Serial1.println("==========================");
// // //   Serial1.println();
// // // }

// // // void loop() {
// // //   while (Serial3.available()) {
// // //     char c = (char)Serial3.read();

// // //     // Optional: echo raw NMEA to Serial1 for reference
// // //     if (SHOW_RAW_NMEA) Serial1.write(c);

// // //     if (c == '\r') continue;
// // //     if (c == '\n') {
// // //       lineBuf[lineLen] = '\0';
// // //       if (lineLen > 6 && lineBuf[0] == '$') {
// // //         // Work on a copy because strtok modifies the string
// // //         char work[128];
// // //         strncpy(work, lineBuf, sizeof(work));
// // //         work[sizeof(work)-1] = '\0';

// // //         if (!strncmp(work, "$GPRMC", 6) || !strncmp(work, "$GNRMC", 6)) {
// // //           handleRMC(work);
// // //         } else if (!strncmp(work, "$GPGGA", 6) || !strncmp(work, "$GNGGA", 6)) {
// // //           handleGGA(work);
// // //         }
// // //       }
// // //       lineLen = 0; // reset for next line
// // //     } else {
// // //       if (lineLen < (int)sizeof(lineBuf) - 1) {
// // //         lineBuf[lineLen++] = c;
// // //       } else {
// // //         // overflow guard
// // //         lineLen = 0;
// // //       }
// // //     }
// // //   }
// // // }

// // // NEO GPS (UART3) -> Serial1 (Bluetooth) CLEAN STATUS OUTPUT
// // // Wiring: NEO TX -> PD9 (RX3), NEO RX -> PD8 (TX3), 3V3 & GND

// // #include <Arduino.h>
// // #include <math.h>

// // HardwareSerial Serial1(PA10, PA9);  // Output / Bluetooth (115200, 8E1)
// // HardwareSerial Serial3(PD9,  PD8);  // GPS (9600, 8N1)

// // // ---- internal state ----
// // static char  lineBuf[128];
// // static int   lineLen = 0;

// // static bool  rmcValid = false;
// // static float latDeg = NAN, lonDeg = NAN;
// // static float sogKn = NAN, cogDeg = NAN;     // speed over ground (knots), course (deg)
// // static char  rmcTime[16] = "";              // hhmmss.sss
// // static char  rmcDate[16] = "";              // ddmmyy

// // static int   fixQ = 0, sats = 0;            // from GGA
// // static float hdop = NAN, altM = NAN;

// // static unsigned long lastPrint = 0;         // status print throttle

// // // ---- helpers ----
// // static float nmeaToDeg(const char* s, char hemi) {
// //   if (!s || !*s) return NAN;
// //   float v = atof(s);
// //   int deg = (int)(v / 100.0f);
// //   float min = v - (deg * 100.0f);
// //   float out = deg + (min / 60.0f);
// //   if (hemi == 'S' || hemi == 'W') out = -out;
// //   return out;
// // }

// // static const char* fixName(int q) {
// //   switch (q) {
// //     case 0: return "No Fix";
// //     case 1: return "GPS Fix";
// //     case 2: return "DGPS Fix";
// //     case 3: return "PPS Fix";
// //     case 4: return "RTK Fixed";
// //     case 5: return "RTK Float";
// //     case 6: return "Dead Reckoning";
// //     default: return "Unknown";
// //   }
// // }

// // static void printOrDashFloat(const char* label, float v, int prec) {
// //   Serial1.print(label);
// //   if (isnan(v)) Serial1.print("-");
// //   else Serial1.print(v, prec);
// // }

// // static void printTimeDate() {
// //   // rmcTime = "hhmmss.sss", rmcDate = "ddmmyy"
// //   if (rmcTime[0]) {
// //     char hh[3]="", mm[3]="", ss[3]="";
// //     strncpy(hh, rmcTime, 2);
// //     strncpy(mm, rmcTime+2, 2);
// //     strncpy(ss, rmcTime+4, 2);
// //     Serial1.print("[GPS] Time: ");
// //     Serial1.print(hh); Serial1.print(":");
// //     Serial1.print(mm); Serial1.print(":");
// //     Serial1.print(ss); Serial1.print("Z");
// //   } else {
// //     Serial1.print("[GPS] Time: -");
// //   }

// //   if (rmcDate[0]) {
// //     char dd[3]="", MM[3]="", yy[3]="";
// //     strncpy(dd, rmcDate, 2);
// //     strncpy(MM, rmcDate+2, 2);
// //     strncpy(yy, rmcDate+4, 2);
// //     Serial1.print(" | Date: 20"); // assumes 20yy
// //     Serial1.print(yy); Serial1.print("-");
// //     Serial1.print(MM); Serial1.print("-");
// //     Serial1.print(dd);
// //   }
// //   Serial1.println();
// // }

// // // ---- parsers ----
// // static void handleRMC(char* s) {
// //   // $GxRMC,1:time,2:status(A/V),3:lat,4:N/S,5:lon,6:E/W,7:sog(kn),8:cog(deg),9:date,...
// //   char* tok[13] = {0};
// //   int n = 0; char* p = strtok(s, ",");
// //   while (p && n < 13) { tok[n++] = p; p = strtok(NULL, ","); }
// //   if (n < 10) return;

// //   rmcValid = (tok[2] && tok[2][0] == 'A');
// //   if (tok[1]) { strncpy(rmcTime, tok[1], sizeof(rmcTime)-1); rmcTime[sizeof(rmcTime)-1] = '\0'; }
// //   if (tok[9]) { strncpy(rmcDate, tok[9], sizeof(rmcDate)-1); rmcDate[sizeof(rmcDate)-1] = '\0'; }

// //   if (rmcValid) {
// //     latDeg = nmeaToDeg(tok[3], tok[4] ? tok[4][0] : 'N');
// //     lonDeg = nmeaToDeg(tok[5], tok[6] ? tok[6][0] : 'E');
// //     sogKn  = (tok[7] && *tok[7]) ? atof(tok[7]) : NAN;
// //     cogDeg = (tok[8] && *tok[8]) ? atof(tok[8]) : NAN;
// //   }
// // }

// // static void handleGGA(char* s) {
// //   // $GxGGA,1:time,2:lat,3:N/S,4:lon,5:E/W,6:fixQ,7:sats,8:HDOP,9:alt,10:M,...
// //   char* tok[15] = {0};
// //   int n = 0; char* p = strtok(s, ",");
// //   while (p && n < 15) { tok[n++] = p; p = strtok(NULL, ","); }
// //   if (n < 10) return;

// //   fixQ = (tok[6] && *tok[6]) ? atoi(tok[6]) : 0;
// //   sats = (tok[7] && *tok[7]) ? atoi(tok[7]) : 0;
// //   hdop = (tok[8] && *tok[8]) ? atof(tok[8]) : NAN;
// //   altM = (tok[9] && *tok[9]) ? atof(tok[9]) : NAN;

// //   // If RMC hasn't given coords yet, GGA has them too:
// //   if (isnan(latDeg) && tok[2] && tok[3]) latDeg = nmeaToDeg(tok[2], tok[3][0]);
// //   if (isnan(lonDeg) && tok[4] && tok[5]) lonDeg = nmeaToDeg(tok[4], tok[5][0]);
// // }

// // // ---- status print ----
// // static void printStatus() {
// //   Serial1.println("========== GPS STATUS ==========");
// //   Serial1.print("[GPS] Fix: "); Serial1.print(fixName(fixQ));
// //   Serial1.print(" ("); Serial1.print(fixQ); Serial1.print(")");
// //   Serial1.print(" | Sats: "); Serial1.print(sats);
// //   Serial1.print(" | HDOP: "); if (isnan(hdop)) Serial1.print("-"); else Serial1.print(hdop, 2);
// //   Serial1.println();

// //   Serial1.print("[GPS] Pos : ");
// //   if (!isnan(latDeg) && !isnan(lonDeg)) {
// //     Serial1.print(latDeg, 6); Serial1.print(", "); Serial1.println(lonDeg, 6);
// //   } else {
// //     Serial1.println("-");
// //   }

// //   Serial1.print("[GPS] Alt : ");
// //   if (isnan(altM)) Serial1.println("-");
// //   else { Serial1.print(altM, 1); Serial1.println(" m"); }

// //   Serial1.print("[GPS] Move: ");
// //   if (isnan(sogKn) && isnan(cogDeg)) {
// //     Serial1.println("-");
// //   } else {
// //     Serial1.print("Speed "); if (isnan(sogKn)) Serial1.print("-"); else Serial1.print(sogKn, 2);
// //     Serial1.print(" kn (");
// //     if (isnan(sogKn)) Serial1.print("-");
// //     else {
// //       float ms = sogKn * 0.514444f;
// //       Serial1.print(ms, 2);
// //     }
// //     Serial1.print(" m/s) | Course ");
// //     if (isnan(cogDeg)) Serial1.println("-");
// //     else { Serial1.print(cogDeg, 2); Serial1.println("°"); }
// //   }

// //   printTimeDate();
// //   Serial1.println("================================\n");
// // }

// // void setup() {
// //   Serial1.begin(115200, SERIAL_8E1);
// //   Serial3.begin(9600);

// //   Serial1.println();
// //   Serial1.println("=== GPS Reader (Clean Output) ===");
// //   Serial1.println("UART3 <= NEO GPS @9600  ->  UART1 (BT) @115200 8E1");
// //   Serial1.println("=================================\n");
// // }

// // void loop() {
// //   while (Serial3.available()) {
// //     char c = (char)Serial3.read();

// //     if (c == '\r') continue;
// //     if (c == '\n') {
// //       lineBuf[lineLen] = '\0';
// //       if (lineLen > 6 && lineBuf[0] == '$') {
// //         char work[128];
// //         strncpy(work, lineBuf, sizeof(work));
// //         work[sizeof(work)-1] = '\0';

// //         if (!strncmp(work, "$GPRMC", 6) || !strncmp(work, "$GNRMC", 6)) {
// //           handleRMC(work);
// //         } else if (!strncmp(work, "$GPGGA", 6) || !strncmp(work, "$GNGGA", 6)) {
// //           handleGGA(work);
// //         }
// //       }
// //       lineLen = 0;
// //     } else {
// //       if (lineLen < (int)sizeof(lineBuf)-1) lineBuf[lineLen++] = c;
// //       else lineLen = 0; // overflow guard
// //     }
// //   }

// //   // print once per second
// //   unsigned long now = millis();
// //   if (now - lastPrint >= 1000) {
// //     printStatus();
// //     lastPrint = now;
// //   }
// // }

// // NEO GPS (UART3) -> Serial1 (115200, 8E1) — Clean, filtered, time-corrected
// // Wiring: NEO TX -> PD9 (RX3), NEO RX -> PD8 (TX3), 3V3 & GND

// #include <Arduino.h>
// #include <math.h>
// #include <string.h>
// #include <stdlib.h>

// HardwareSerial Serial1(PA10, PA9);   // Output / Bluetooth (115200, 8E1)
// HardwareSerial Serial3(PD9,  PD8);   // GPS (9600, 8N1)

// // ---------- Tunables ----------
// static const float HDOP_MAX_FOR_SMOOTH = 2.5f; // only smooth when geometry is decent
// static const float EMA_ALPHA = 0.25f;          // smoothing factor for lat/lon
// static const float SPEED_MPS_NOISE = 0.50f;    // below this, treat as 0 (stationary)
// static const float HEADING_MIN_MPS = 1.00f;    // need >= this to trust heading
// static const int   TZ_OFFSET_MIN = +240;       // Dubai = UTC+4h => 4*60

// // ---------- State ----------
// static char  lineBuf[128];  static int lineLen = 0;
// static bool  haveRMC = false, haveGGA = false;

// static float latDeg = NAN, lonDeg = NAN;     // raw last parsed
// static float latSm  = NAN, lonSm  = NAN;     // smoothed
// static float sogKn = NAN, cogDeg = NAN;      // speed(kn), course(deg)
// static int   fixQ = 0, sats = 0;
// static float hdop = NAN, altMSL = NAN, geoidSep = NAN; // GGA fields
// static unsigned long lastPrint = 0;

// // RMC time/date
// static int utc_h=-1, utc_m=-1, utc_s=-1;
// static int utc_d=-1, utc_M=-1, utc_y=-1; // yy (two-digit) -> will print as 20yy

// // ---------- Helpers ----------
// static float nmeaToDeg(const char* s, char hemi) {
//   if (!s || !*s) return NAN;
//   float v = atof(s);
//   int deg = (int)(v / 100.0f);
//   float min = v - (deg * 100.0f);
//   float out = deg + (min / 60.0f);
//   if (hemi == 'S' || hemi == 'W') out = -out;
//   return out;
// }

// static void emaUpdate(float inLat, float inLon, float currHdop) {
//   if (isnan(inLat) || isnan(inLon)) return;
//   if (isnan(latSm) || isnan(lonSm) || isnan(currHdop) || currHdop > HDOP_MAX_FOR_SMOOTH) {
//     // initialize or poor geometry: jump to current (no smoothing)
//     latSm = inLat; lonSm = inLon;
//   } else {
//     latSm = EMA_ALPHA * inLat + (1.0f - EMA_ALPHA) * latSm;
//     lonSm = EMA_ALPHA * inLon + (1.0f - EMA_ALPHA) * lonSm;
//   }
// }

// static void parseUTC(const char* tHHMMSS, const char* dDDMMYY) {
//   // Time "hhmmss.sss", Date "ddmmyy"
//   if (tHHMMSS && strlen(tHHMMSS) >= 6) {
//     utc_h = (tHHMMSS[0]-'0')*10 + (tHHMMSS[1]-'0');
//     utc_m = (tHHMMSS[2]-'0')*10 + (tHHMMSS[3]-'0');
//     utc_s = (tHHMMSS[4]-'0')*10 + (tHHMMSS[5]-'0');
//   }
//   if (dDDMMYY && strlen(dDDMMYY) >= 6) {
//     utc_d = (dDDMMYY[0]-'0')*10 + (dDDMMYY[1]-'0');
//     utc_M = (dDDMMYY[2]-'0')*10 + (dDDMMYY[3]-'0');
//     utc_y = (dDDMMYY[4]-'0')*10 + (dDDMMYY[5]-'0'); // 00..99
//   }
// }

// static bool isLeap(int y) { // y full (e.g., 2025)
//   return ((y%4==0) && (y%100!=0)) || (y%400==0);
// }
// static int daysInMonth(int y, int m) {
//   static const int dm[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
//   if (m==2) return dm[m-1] + (isLeap(y)?1:0);
//   return dm[m-1];
// }

// static void printTimeUTCandLocal() {
//   Serial1.print("[GPS] Time (UTC): ");
//   if (utc_h>=0) {
//     if (utc_h<10) Serial1.print('0'); Serial1.print(utc_h); Serial1.print(':');
//     if (utc_m<10) Serial1.print('0'); Serial1.print(utc_m); Serial1.print(':');
//     if (utc_s<10) Serial1.print('0'); Serial1.print(utc_s); Serial1.print('Z');
//   } else Serial1.print('-');

//   Serial1.print(" | Date (UTC): ");
//   if (utc_d>=0) {
//     int fullY = 2000 + (utc_y<80 ? utc_y : utc_y); // assume 2000..2079
//     Serial1.print("20"); if (utc_y<10) Serial1.print('0'); Serial1.print(utc_y);
//     Serial1.print('-'); if (utc_M<10) Serial1.print('0'); Serial1.print(utc_M);
//     Serial1.print('-'); if (utc_d<10) Serial1.print('0'); Serial1.print(utc_d);

//     // Local time
//     int Y = fullY, M = utc_M, D = utc_d;
//     int totalMin = utc_h*60 + utc_m + TZ_OFFSET_MIN;
//     if (totalMin < 0) { totalMin += 1440; D -= 1; }
//     if (totalMin >= 1440) { totalMin -= 1440; D += 1; }
//     int H = totalMin / 60; int Min = totalMin % 60; int S = utc_s;

//     // adjust date if crossed day
//     if (D <= 0) {
//       M -= 1; if (M <= 0) { M = 12; Y -= 1; }
//       D = daysInMonth(Y, M);
//     } else {
//       int dim = daysInMonth(Y, M);
//       if (D > dim) { D = 1; M += 1; if (M > 12) { M = 1; Y += 1; } }
//     }

//     Serial1.print("  |  Local (UTC+4): ");
//     if (H<10) Serial1.print('0'); Serial1.print(H); Serial1.print(':');
//     if (Min<10) Serial1.print('0'); Serial1.print(Min); Serial1.print(':');
//     if (S<10) Serial1.print('0'); Serial1.print(S);
//     Serial1.print("  "); Serial1.print(Y); Serial1.print('-');
//     if (M<10) Serial1.print('0'); Serial1.print(M); Serial1.print('-');
//     if (D<10) Serial1.print('0'); Serial1.println(D);
//   } else {
//     Serial1.println('-');
//   }
// }

// // ---------- Parsers ----------
// static void handleRMC(char* s) {
//   // $GxRMC,1:time,2:status(A/V),3:lat,4:N/S,5:lon,6:E/W,7:sog(kn),8:cog(deg),9:date,...
//   char* tok[13] = {0};
//   int n = 0; char* p = strtok(s, ",");
//   while (p && n < 13) { tok[n++] = p; p = strtok(NULL, ","); }
//   if (n < 10) return;

//   bool valid = (tok[2] && tok[2][0] == 'A');
//   if (tok[1] && tok[9]) parseUTC(tok[1], tok[9]);

//   if (valid) {
//     float la = nmeaToDeg(tok[3], tok[4] ? tok[4][0] : 'N');
//     float lo = nmeaToDeg(tok[5], tok[6] ? tok[6][0] : 'E');
//     if (!isnan(la) && !isnan(lo)) { latDeg = la; lonDeg = lo; }
//     sogKn  = (tok[7] && *tok[7]) ? atof(tok[7]) : NAN; // knots
//     cogDeg = (tok[8] && *tok[8]) ? atof(tok[8]) : NAN; // degrees
//     haveRMC = true;
//   }
// }

// static void handleGGA(char* s) {
//   // $GxGGA,1:time,2:lat,3:N/S,4:lon,5:E/W,6:fixQ,7:sats,8:HDOP,9:altMSL,10:M,11:geoid,12:M,...
//   char* tok[16] = {0};
//   int n = 0; char* p = strtok(s, ",");
//   while (p && n < 16) { tok[n++] = p; p = strtok(NULL, ","); }
//   if (n < 10) return;

//   fixQ = (tok[6] && *tok[6]) ? atoi(tok[6]) : 0;
//   sats = (tok[7] && *tok[7]) ? atoi(tok[7]) : 0;
//   hdop = (tok[8] && *tok[8]) ? atof(tok[8]) : NAN;
//   altMSL   = (tok[9]  && *tok[9])  ? atof(tok[9])  : NAN;
//   geoidSep = (tok[11] && *tok[11]) ? atof(tok[11]) : NAN;

//   // Also get coords if RMC hasn't set them yet
//   if ((isnan(latDeg) || isnan(lonDeg)) && tok[2] && tok[3] && tok[4] && tok[5]) {
//     float la = nmeaToDeg(tok[2], tok[3][0]);
//     float lo = nmeaToDeg(tok[4], tok[5][0]);
//     if (!isnan(la) && !isnan(lo)) { latDeg = la; lonDeg = lo; }
//   }
//   haveGGA = true;
// }

// // ---------- Presentation ----------
// static void printStatus() {
//   // Smooth position if we have fresh data
//   if (!isnan(latDeg) && !isnan(lonDeg)) emaUpdate(latDeg, lonDeg, hdop);

//   // Compute speed and heading display with noise gating
//   float speed_mps = NAN;
//   if (!isnan(sogKn)) speed_mps = sogKn * 0.514444f;

//   bool stationary = (!isnan(speed_mps) && speed_mps < SPEED_MPS_NOISE);

//   Serial1.println("========== GPS STATUS ==========");
//   Serial1.print("[GPS] Fix: ");
//   switch (fixQ) {
//     case 0: Serial1.print("No Fix"); break;
//     case 1: Serial1.print("GPS Fix"); break;
//     case 2: Serial1.print("DGPS Fix"); break;
//     default: Serial1.print("FixQ="); Serial1.print(fixQ);
//   }
//   Serial1.print(" | Sats: "); Serial1.print(sats);
//   Serial1.print(" | HDOP: "); if (isnan(hdop)) Serial1.print("-"); else Serial1.print(hdop, 2);
//   Serial1.println();

//   Serial1.print("[GPS] Pos : ");
//   if (!isnan(latSm) && !isnan(lonSm)) {
//     Serial1.print(latSm, 6); Serial1.print(", "); Serial1.println(lonSm, 6);
//   } else if (!isnan(latDeg) && !isnan(lonDeg)) {
//     Serial1.print(latDeg, 6); Serial1.print(", "); Serial1.println(lonDeg, 6);
//   } else {
//     Serial1.println("-");
//   }

//   Serial1.print("[GPS] Alt : ");
//   if (isnan(altMSL)) {
//     Serial1.println("-");
//   } else {
//     Serial1.print(altMSL, 1); Serial1.print(" m MSL");
//     if (!isnan(geoidSep)) {
//       float h_ellipsoid = altMSL + geoidSep; // WGS-84 ellipsoidal height
//       Serial1.print("  |  WGS84: "); Serial1.print(h_ellipsoid, 1); Serial1.print(" m");
//     }
//     Serial1.println();
//   }

//   Serial1.print("[GPS] Move: ");
//   if (isnan(speed_mps)) {
//     Serial1.println("-");
//   } else if (stationary) {
//     Serial1.println("Stationary (filtered noise)");
//   } else {
//     Serial1.print("Speed "); Serial1.print(sogKn, 2); Serial1.print(" kn (");
//     Serial1.print(speed_mps, 2); Serial1.print(" m/s)");
//     Serial1.print(" | Course ");
//     if (!isnan(cogDeg) && speed_mps >= HEADING_MIN_MPS) {
//       Serial1.print(cogDeg, 1); Serial1.println("°");
//     } else {
//       Serial1.println("- (too slow for reliable heading)");
//     }
//   }

//   printTimeUTCandLocal();
//   Serial1.println("================================\n");
// }

// void setup() {
//   Serial1.begin(115200, SERIAL_8E1);
//   Serial3.begin(9600);

//   Serial1.println();
//   Serial1.println("=== GPS Reader (Clean + Filters + Local Time) ===");
//   Serial1.println("UART3 <= NEO GPS @9600  ->  UART1 (BT) @115200 8E1");
//   Serial1.println("Noise-gated speed, heading only when moving, EMA-smoothed lat/lon.");
//   Serial1.println("=================================================\n");
// }

// void loop() {
//   while (Serial3.available()) {
//     char c = (char)Serial3.read();
//     if (c == '\r') continue;
//     if (c == '\n') {
//       lineBuf[lineLen] = '\0';
//       if (lineLen > 6 && lineBuf[0] == '$') {
//         char work[128];
//         strncpy(work, lineBuf, sizeof(work));
//         work[sizeof(work)-1] = '\0';

//         if (!strncmp(work, "$GPRMC", 6) || !strncmp(work, "$GNRMC", 6)) {
//           handleRMC(work);
//         } else if (!strncmp(work, "$GPGGA", 6) || !strncmp(work, "$GNGGA", 6)) {
//           handleGGA(work);
//         }
//       }
//       lineLen = 0;
//     } else {
//       if (lineLen < (int)sizeof(lineBuf) - 1) lineBuf[lineLen++] = c;
//       else lineLen = 0; // overflow guard
//     }
//   }

//   if (millis() - lastPrint >= 1000) {
//     printStatus();
//     lastPrint = millis();
//   }
// }

// NEO GPS (UART3) -> Serial1 (115200, 8E1) — Clean timestamps (UTC + Dubai), filtered speed/course,
// MSL + WGS84 altitude, tidy status.
// Wiring: NEO TX -> PD9 (RX3), NEO RX -> PD8 (TX3), 3V3 & GND

#include <Arduino.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

HardwareSerial Serial1(PA10, PA9);   // Output / Bluetooth (115200, 8E1)
HardwareSerial Serial3(PD9,  PD8);   // GPS (9600, 8N1)

// ---- Tunables ----
static const int   TZ_OFFSET_MIN      = +240;  // Dubai = UTC+4h = 240 minutes
static const float SPEED_NOISE_MPS    = 0.50f; // < 0.5 m/s -> treat as 0
static const float HEADING_MIN_MPS    = 1.00f; // need >= 1 m/s to trust course
static const float HDOP_SMOOTH_MAX    = 2.5f;  // only smooth position when HDOP is decent
static const float EMA_ALPHA          = 0.25f; // smoothing factor for lat/lon
static const uint32_t PRINT_PERIOD_MS = 1000;  // print once per second

// ---- State ----
static char  lineBuf[128];  static int lineLen = 0;
static bool  haveRMC = false, haveGGA = false;

static float latDeg = NAN, lonDeg = NAN;     // latest parsed
static float latSm  = NAN, lonSm  = NAN;     // smoothed
static float sogKn = NAN, cogDeg = NAN;      // speed(knots), course(deg)
static int   fixQ = 0, sats = 0;             // GGA: fix quality, satellites used
static float hdop = NAN, altMSL = NAN, geoidSep = NAN; // GGA: MSL alt + geoid separation

// UTC date/time from RMC
static int utc_h=-1, utc_m=-1, utc_s=-1;
static int utc_d=-1, utc_M=-1, utc_y=-1; // two-digit year (00..99)

static uint32_t lastPrint = 0;

// ---- Helpers ----
static float nmeaToDeg(const char* s, char hemi) {
  if (!s || !*s) return NAN;
  float v = atof(s);
  int deg = (int)(v / 100.0f);
  float min = v - (deg * 100.0f);
  float out = deg + (min / 60.0f);
  if (hemi == 'S' || hemi == 'W') out = -out;
  return out;
}

static void emaUpdate(float la, float lo, float currHdop) {
  if (isnan(la) || isnan(lo)) return;
  if (isnan(latSm) || isnan(lonSm) || isnan(currHdop) || currHdop > HDOP_SMOOTH_MAX) {
    latSm = la; lonSm = lo; // no smoothing on first run or bad geometry
  } else {
    latSm = EMA_ALPHA * la + (1.0f - EMA_ALPHA) * latSm;
    lonSm = EMA_ALPHA * lo + (1.0f - EMA_ALPHA) * lonSm;
  }
}

static void parseUTC(const char* tHHMMSS, const char* dDDMMYY) {
  // Time "hhmmss.sss", Date "ddmmyy"
  if (tHHMMSS && strlen(tHHMMSS) >= 6) {
    utc_h = (tHHMMSS[0]-'0')*10 + (tHHMMSS[1]-'0');
    utc_m = (tHHMMSS[2]-'0')*10 + (tHHMMSS[3]-'0');
    utc_s = (tHHMMSS[4]-'0')*10 + (tHHMMSS[5]-'0');
  }
  if (dDDMMYY && strlen(dDDMMYY) >= 6) {
    utc_d = (dDDMMYY[0]-'0')*10 + (dDDMMYY[1]-'0');
    utc_M = (dDDMMYY[2]-'0')*10 + (dDDMMYY[3]-'0');
    utc_y = (dDDMMYY[4]-'0')*10 + (dDDMMYY[5]-'0'); // 00..99
  }
}

static bool isLeap(int y) { return ((y%4==0) && (y%100!=0)) || (y%400==0); }
static int dim(int y,int m){ static const int d[12]={31,28,31,30,31,30,31,31,30,31,30,31};
  return m==2 ? d[m-1] + (isLeap(y)?1:0) : d[m-1];
}

static void printTimeUTCandDubai() {
  // UTC
  Serial1.print("[GPS] Time UTC : ");
  if (utc_h>=0) {
    if (utc_h<10) Serial1.print('0'); Serial1.print(utc_h); Serial1.print(':');
    if (utc_m<10) Serial1.print('0'); Serial1.print(utc_m); Serial1.print(':');
    if (utc_s<10) Serial1.print('0'); Serial1.print(utc_s); Serial1.print('Z');
  } else Serial1.print('-');

  Serial1.print(" | Date UTC : ");
  if (utc_d>=0) {
    int fullY = 2000 + utc_y; // assume 2000..2099
    Serial1.print(fullY); Serial1.print('-');
    if (utc_M<10) Serial1.print('0'); Serial1.print(utc_M); Serial1.print('-');
    if (utc_d<10) Serial1.print('0'); Serial1.print(utc_d);

    // Local Dubai time = UTC + 4h
    int Y=fullY, M=utc_M, D=utc_d;
    int totalMin = utc_h*60 + utc_m + TZ_OFFSET_MIN;
    if (totalMin < 0) { totalMin += 1440; D -= 1; }
    if (totalMin >= 1440) { totalMin -= 1440; D += 1; }
    int H = totalMin / 60; int Min = totalMin % 60; int S = utc_s;

    // adjust date if rolled over
    if (D<=0){ M-=1; if(M<=0){M=12; Y-=1;} D=dim(Y,M); }
    int daysM = dim(Y,M);
    if (D>daysM){ D=1; M+=1; if(M>12){M=1; Y+=1;} }

    Serial1.print("  |  Dubai (UTC+4): ");
    if (H<10) Serial1.print('0'); Serial1.print(H); Serial1.print(':');
    if (Min<10) Serial1.print('0'); Serial1.print(Min); Serial1.print(':');
    if (S<10) Serial1.print('0'); Serial1.print(S);
    Serial1.print("  "); Serial1.print(Y); Serial1.print('-');
    if (M<10) Serial1.print('0'); Serial1.print(M); Serial1.print('-');
    if (D<10) Serial1.print('0'); Serial1.print(D);
  } else {
    Serial1.print('-');
  }
  Serial1.println();
}

// ---- Parsers ----
static void handleRMC(char* s) {
  // $GxRMC,1:time,2:status(A/V),3:lat,4:N/S,5:lon,6:E/W,7:sog(kn),8:cog(deg),9:date,...
  char* tok[13] = {0};
  int n = 0; char* p = strtok(s, ",");
  while (p && n < 13) { tok[n++] = p; p = strtok(NULL, ","); }
  if (n < 10) return;

  bool valid = (tok[2] && tok[2][0] == 'A');
  if (tok[1] && tok[9]) parseUTC(tok[1], tok[9]);

  if (valid) {
    float la = nmeaToDeg(tok[3], tok[4] ? tok[4][0] : 'N');
    float lo = nmeaToDeg(tok[5], tok[6] ? tok[6][0] : 'E');
    if (!isnan(la) && !isnan(lo)) { latDeg = la; lonDeg = lo; }
    sogKn  = (tok[7] && *tok[7]) ? atof(tok[7]) : NAN;
    cogDeg = (tok[8] && *tok[8]) ? atof(tok[8]) : NAN;
    haveRMC = true;
  }
}

static void handleGGA(char* s) {
  // $GxGGA,1:time,2:lat,3:N/S,4:lon,5:E/W,6:fixQ,7:sats,8:HDOP,9:altMSL,10:M,11:geoid,12:M,...
  char* tok[16] = {0};
  int n = 0; char* p = strtok(s, ",");
  while (p && n < 16) { tok[n++] = p; p = strtok(NULL, ","); }
  if (n < 10) return;

  fixQ = (tok[6] && *tok[6]) ? atoi(tok[6]) : 0;
  sats = (tok[7] && *tok[7]) ? atoi(tok[7]) : 0;
  hdop = (tok[8] && *tok[8]) ? atof(tok[8]) : NAN;
  altMSL   = (tok[9]  && *tok[9])  ? atof(tok[9])  : NAN;
  geoidSep = (tok[11] && *tok[11]) ? atof(tok[11]) : NAN;

  // fall-back coordinates if RMC hasn't set them
  if ((isnan(latDeg) || isnan(lonDeg)) && tok[2] && tok[3] && tok[4] && tok[5]) {
    float la = nmeaToDeg(tok[2], tok[3][0]);
    float lo = nmeaToDeg(tok[4], tok[5][0]);
    if (!isnan(la) && !isnan(lo)) { latDeg = la; lonDeg = lo; }
  }
  haveGGA = true;
}

// ---- Presentation ----
static void printStatus() {
  if (!isnan(latDeg) && !isnan(lonDeg)) emaUpdate(latDeg, lonDeg, hdop);

  float speed_mps = (!isnan(sogKn)) ? (sogKn * 0.514444f) : NAN;
  bool stationary = (!isnan(speed_mps) && speed_mps < SPEED_NOISE_MPS);

  Serial1.println("========== GPS STATUS ==========");
  Serial1.print("[GPS] Fix: ");
  switch (fixQ) {
    case 0: Serial1.print("No Fix"); break;
    case 1: Serial1.print("GPS Fix"); break;
    case 2: Serial1.print("DGPS Fix"); break;
    default: Serial1.print("FixQ="); Serial1.print(fixQ);
  }
  Serial1.print(" | Sats: "); Serial1.print(sats);
  Serial1.print(" | HDOP: "); if (isnan(hdop)) Serial1.print("-"); else Serial1.print(hdop, 2);
  Serial1.println();

  Serial1.print("[GPS] Pos : ");
  if (!isnan(latSm) && !isnan(lonSm)) {
    Serial1.print(latSm, 6); Serial1.print(", "); Serial1.println(lonSm, 6);
  } else if (!isnan(latDeg) && !isnan(lonDeg)) {
    Serial1.print(latDeg, 6); Serial1.print(", "); Serial1.println(lonDeg, 6);
  } else Serial1.println("-");

  Serial1.print("[GPS] Alt : ");
  if (isnan(altMSL)) {
    Serial1.println("-");
  } else {
    Serial1.print(altMSL, 1); Serial1.print(" m MSL");
    if (!isnan(geoidSep)) {
      float h_ellipsoid = altMSL + geoidSep; // WGS-84 height
      Serial1.print("  |  WGS84: "); Serial1.print(h_ellipsoid, 1); Serial1.print(" m");
    }
    Serial1.println();
  }

  Serial1.print("[GPS] Move: ");
  if (isnan(speed_mps) || stationary) {
    Serial1.println("Stationary (noise filtered)");
  } else {
    Serial1.print("Speed "); Serial1.print(sogKn, 2); Serial1.print(" kn (");
    Serial1.print(speed_mps, 2); Serial1.print(" m/s)");
    Serial1.print(" | Course ");
    if (!isnan(cogDeg) && speed_mps >= HEADING_MIN_MPS) {
      Serial1.print(cogDeg, 1); Serial1.println("°");
    } else {
      Serial1.println("- (too slow for reliable heading)");
    }
  }

  printTimeUTCandDubai();
  Serial1.println("================================\n");
}

void setup() {
  Serial1.begin(115200, SERIAL_8E1);
  Serial3.begin(9600);

  Serial1.println();
  Serial1.println("=== GPS Reader (Clean + Local Time + Filters) ===");
  Serial1.println("UART3 <= NEO GPS @9600  ->  UART1 (BT) @115200 8E1");
  Serial1.println("UTC + Dubai time, MSL+WGS84 altitude, speed/course noise gating.");
  Serial1.println("=================================================\n");
}

void loop() {
  while (Serial3.available()) {
    char c = (char)Serial3.read();
    if (c == '\r') continue;
    if (c == '\n') {
      lineBuf[lineLen] = '\0';
      if (lineLen > 6 && lineBuf[0] == '$') {
        char work[128]; strncpy(work, lineBuf, sizeof(work)); work[sizeof(work)-1] = '\0';
        if (!strncmp(work, "$GPRMC", 6) || !strncmp(work, "$GNRMC", 6)) handleRMC(work);
        else if (!strncmp(work, "$GPGGA", 6) || !strncmp(work, "$GNGGA", 6)) handleGGA(work);
      }
      lineLen = 0;
    } else {
      if (lineLen < (int)sizeof(lineBuf) - 1) lineBuf[lineLen++] = c;
      else lineLen = 0; // overflow guard
    }
  }

  if (millis() - lastPrint >= PRINT_PERIOD_MS) {
    printStatus();
    lastPrint = millis();
  }
}


