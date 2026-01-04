
// // // --- Libraries ---
// // #include <Arduino.h>
// // #include <LSM6DS3.h>   // Accel + Gyro
// // #include <LIS3MDL.h>   // Magnetometer
// // #include <LM75A.h>     // Temperature (ambient)
// // #include <MS5611.h>    // Barometer (temp, pressure, height)

// // // --- Namespace (as per your format) ---
// // using namespace IntroStratLib;

// // // --- UART ---
// // HardwareSerial Serial1(PA10, PA9);  // Bluetooth / Telemetry

// // // --- I2C Buses ---
// // TwoWire Wire1(PB7, PB6);   // Internal sensors I2C
// // TwoWire Wire4(PD13, PD12); // External sensors I2C (reserved/available)

// // // --- Sensor Objects (all on Wire1 here) ---
// // LSM6DS3 imu(Wire1, 0x6A);     // Accel + Gyro (same chip)
// // LIS3MDL mag(Wire1, 0x1C);     // Magnetometer  (use 0x1E if strapped)
// // LM75A   tempLM75(Wire1, 0x4A);// Temperature sensor
// // MS5611  baro(Wire1, 0x77);    // Barometer

// // // --- Global Readings ---
// // float Ax = 0.0f, Ay = 0.0f, Az = 0.0f;         // Accel
// // float gx = 0.0f, gy = 0.0f, gz = 0.0f;         // Gyro
// // float Mx = 0.0f, My = 0.0f, Mz = 0.0f;         // Magnetometer
// // float T_LM75 = 0.0f, T8_LM75 = 0.0f;           // LM75 (°C and °C*8)
// // float T_MS = 0.0f, P_MS = 0.0f, H_MS = 0.0f;   // MS5611 (°C, mbar/Pa, m)

// // // --- Status Flags (like your sample style) ---
// // bool ACCEL_FLAG = false;
// // bool GYRO_FLAG  = false;
// // bool MAG_FLAG   = false;
// // bool TEMP_FLAG  = false;
// // bool BARO_FLAG  = false;

// // // --- Prototypes ---
// // void InitSensors(void);
// // void ReadAccel(void);
// // void ReadGyro(void);
// // void ReadMag(void);
// // void ReadTempLM75(void);
// // void ReadBaroMS5611(void);
// // void send_data(void);
// // void print_debug_csv(void);

// // // ============================================================================
// // // Setup / Loop
// // // ============================================================================
// // void setup() {
// //   Serial1.begin(115200, SERIAL_8E1);   // Your parity/baud style
// //   Wire1.begin();                       // Internal I2C
// //   Wire4.begin();                       // External I2C (available)
// //   delay(50);

// //   InitSensors();
// // }

// // void loop() {
// //   ReadAccel();
// //   ReadGyro();
// //   ReadMag();
// //   ReadTempLM75();
// //   ReadBaroMS5611();

// //   // Compact framed packet for downstream parser
// //   send_data();

// //   // Optional: verbose human-readable CSV for debugging/logging
// //   // print_debug_csv();

// //   delay(100); // Adjust rate as needed
// // }

// // // ============================================================================
// // // Init block
// // // ============================================================================
// // void InitSensors(void) {
// //   // LSM6DS3 Accel
// //   delay(100);
// //   imu.InitAccel();
// //   ACCEL_FLAG = true;
// //   delay(50);

// //   // LSM6DS3 Gyro
// //   imu.InitGyro();
// //   GYRO_FLAG = true;
// //   delay(50);

// //   // LIS3MDL Magnetometer
// //   mag.Init();
// //   MAG_FLAG = true;
// //   delay(50);

// //   // LM75A Temperature
// //   tempLM75.Init();
// //   TEMP_FLAG = true;
// //   delay(50);

// //   // MS5611 Barometer
// //   baro.Init();
// //   BARO_FLAG = true;
// //   delay(50);
// // }

// // // ============================================================================
// // // Read blocks (one per sensor, modular)
// // // ============================================================================
// // void ReadAccel(void) {
// //   // Using your axis-call style; keep it consistent with your past code if needed
// //   Ax = imu.AZ();
// //   Ay = imu.AY();
// //   Az = imu.AX();
// // }

// // void ReadGyro(void) {
// //   gx = imu.GX();
// //   gy = imu.GY();
// //   gz = imu.GZ();
// // }

// // void ReadMag(void) {
// //   Mx = mag.Mx();
// //   My = mag.My();
// //   Mz = mag.Mz();
// // }

// // void ReadTempLM75(void) {
// //   // Two representations (°C and °C*8) as in your base code
// //   T_LM75 = tempLM75.GetTemperature();
// //   T8_LM75 = tempLM75.GetTemperatureTimes8();
// // }

// // void ReadBaroMS5611(void) {
// //   T_MS = baro.GetTemperature();  // °C
// //   P_MS = baro.GetPressure();     // Library units (often Pa or mbar; match your lib)
// //   H_MS = baro.GetHeight();       // Derived altitude (m)
// // }

// // // ============================================================================
// // // Framed compact sender (fixed field order; easy to parse)
// // // ============================================================================
// // void send_data(void) {
// //   Serial1.print('*');

// //   // Flags first (1/0) — consistent header
// //   Serial1.print(ACCEL_FLAG ? '1' : '0'); Serial1.print(',');
// //   Serial1.print(MAG_FLAG   ? '1' : '0'); Serial1.print(',');
// //   Serial1.print(GYRO_FLAG  ? '1' : '0'); Serial1.print(',');
// //   Serial1.print(TEMP_FLAG  ? '1' : '0'); Serial1.print(',');
// //   Serial1.print(BARO_FLAG  ? '1' : '0'); Serial1.print(',');

// //   // Accel
// //   if (ACCEL_FLAG) { Serial1.print(Ax,3); Serial1.print(','); Serial1.print(Ay,3); Serial1.print(','); Serial1.print(Az,3); Serial1.print(','); }
// //   else            { Serial1.print("0.000,0.000,0.000,"); }

// //   // Mag
// //   if (MAG_FLAG)   { Serial1.print(Mx,3); Serial1.print(','); Serial1.print(My,3); Serial1.print(','); Serial1.print(Mz,3); Serial1.print(','); }
// //   else            { Serial1.print("0.000,0.000,0.000,"); }

// //   // Gyro
// //   if (GYRO_FLAG)  { Serial1.print(gx,3); Serial1.print(','); Serial1.print(gy,3); Serial1.print(','); Serial1.print(gz,3); Serial1.print(','); }
// //   else            { Serial1.print("0.000,0.000,0.000,"); }

// //   // LM75 (Temperature + Temperature*8)
// //   if (TEMP_FLAG)  { Serial1.print(T_LM75,2); Serial1.print(','); Serial1.print(T8_LM75,2); Serial1.print(','); }
// //   else            { Serial1.print("0.00,0.00,"); }

// //   // MS5611 (Temp, Pressure, Height)
// //   if (BARO_FLAG)  { Serial1.print(T_MS,2); Serial1.print(','); Serial1.print(P_MS,2); Serial1.print(','); Serial1.print(H_MS,2); }
// //   else            { Serial1.print("0.00,0.00,0.00"); }

// //   Serial1.println('#');
// // }

// // // ============================================================================
// // // Optional CSV debug (human-readable, easy to paste into logs)
// // // ============================================================================
// // void print_debug_csv(void) {
// //   Serial1.print("FLAGS: A=");
// //   Serial1.print(ACCEL_FLAG ? "1" : "0");
// //   Serial1.print(", M=");
// //   Serial1.print(MAG_FLAG   ? "1" : "0");
// //   Serial1.print(", G=");
// //   Serial1.print(GYRO_FLAG  ? "1" : "0");
// //   Serial1.print(", T=");
// //   Serial1.print(TEMP_FLAG  ? "1" : "0");
// //   Serial1.print(", B=");
// //   Serial1.println(BARO_FLAG ? "1" : "0");

// //   Serial1.print("ACCEL,"); Serial1.print(Ax); Serial1.print(','); Serial1.print(Ay); Serial1.print(','); Serial1.println(Az);
// //   Serial1.print("MAG,");   Serial1.print(Mx); Serial1.print(','); Serial1.print(My); Serial1.print(','); Serial1.println(Mz);
// //   Serial1.print("GYRO,");  Serial1.print(gx); Serial1.print(','); Serial1.print(gy); Serial1.print(','); Serial1.println(gz);
// //   Serial1.print("LM75,");  Serial1.print(T_LM75); Serial1.print(','); Serial1.println(T8_LM75);
// //   Serial1.print("MS5611,");Serial1.print(T_MS); Serial1.print(','); Serial1.print(P_MS); Serial1.print(','); Serial1.println(H_MS);
// // }

// // ============================================================================
// // Full code integration with Internal/External sensors + SD CSV Logging
// // Modular function blocks (Init / Read / Send / Log) - STM32 custom board
// // UART:  Serial1(PA10, PA9)  |  I2C: Wire1(PB7,PB6) + Wire4(PD13,PD12)
// // SDIO:  STM32SD with remapped pins (PC8..PC12, PD2) and detect pin PB8
// //
// // Sensors (I2C on Wire1 unless noted):
// //   - LSM6DS3 (Accel + Gyro) @ 0x6A
// //   - LIS3MDL (Magnetometer) @ 0x1C (alt 0x1E)
// //   - LM75A   (Temperature)  @ 0x4A
// //   - MS5611  (Barometer)    @ 0x77
// //
// // CSV file: sensors.csv
// // Columns:
// //   timestamp_ms,
// //   ACCEL_FLAG,MAG_FLAG,GYRO_FLAG,TEMP_FLAG,BARO_FLAG,
// //   Ax,Ay,Az, Mx,My,Mz, gx,gy,gz, T_LM75,T8_LM75, T_MS,P_MS,H_MS
// //
// // Logging frequency: every 5000 ms (5 s). Sensing & serial can run faster.
// // ============================================================================

// // --- Libraries ---
// #include <Arduino.h>
// #include <LSM6DS3.h>   // Accel + Gyro
// #include <LIS3MDL.h>   // Magnetometer
// #include <LM75A.h>     // Temperature (ambient)
// #include <MS5611.h>    // Barometer (temp, pressure, height)
// #include <STM32SD.h>   // SD card (STM32)

// // --- Namespace (as per your format) ---
// using namespace IntroStratLib;

// // --- UART ---
// HardwareSerial Serial1(PA10, PA9);  // Bluetooth / Telemetry

// // --- I2C Buses ---
// TwoWire Wire1(PB7, PB6);   // Internal sensors I2C
// TwoWire Wire4(PD13, PD12); // External sensors I2C (reserved/available)

// // --- Sensor Objects (all on Wire1 here) ---
// LSM6DS3 imu(Wire1, 0x6A);     // Accel + Gyro (same chip)
// LIS3MDL mag(Wire1, 0x1C);     // Magnetometer  (use 0x1E if strapped)
// LM75A   tempLM75(Wire1, 0x4A);// Temperature sensor
// MS5611  baro(Wire1, 0x77);    // Barometer

// // --- Global Readings ---
// float Ax = 0.0f, Ay = 0.0f, Az = 0.0f;         // Accel
// float gx = 0.0f, gy = 0.0f, gz = 0.0f;         // Gyro
// float Mx = 0.0f, My = 0.0f, Mz = 0.0f;         // Magnetometer
// float T_LM75 = 0.0f, T8_LM75 = 0.0f;           // LM75 (°C and °C*8)
// float T_MS = 0.0f, P_MS = 0.0f, H_MS = 0.0f;   // MS5611 (°C, mbar/Pa, m)

// // --- Status Flags (like your sample style) ---
// bool ACCEL_FLAG = false;
// bool GYRO_FLAG  = false;
// bool MAG_FLAG   = false;
// bool TEMP_FLAG  = false;
// bool BARO_FLAG  = false;

// // --- SD Card (pins + file) ---
// #ifndef SD_DETECT_PIN
// #define SD_DETECT_PIN PB8   // Detect pin
// #endif

// // SDIO remap pins (per your example)
// #define SD_D0   PC8
// #define SD_D1   PC9
// #define SD_D2   PC10
// #define SD_D3   PC11
// #define SD_CMD  PD2
// #define SD_CK   PC12

// File SensorData;
// const char* kLogFile = "sensors.csv";

// // --- Logging cadence ---
// const uint32_t LOG_INTERVAL_MS = 5000;  // 5 seconds
// uint32_t lastLogMs = 0;

// // --- Prototypes ---
// void InitSensors(void);
// void ReadAccel(void);
// void ReadGyro(void);
// void ReadMag(void);
// void ReadTempLM75(void);
// void ReadBaroMS5611(void);
// void send_data(void);
// void print_debug_csv(void);

// // SD helpers
// bool InitSD(void);
// bool OpenLogIfNeeded(void);
// void WriteCSVHeaderIfNew(File &f);
// void LogToSD(void);

// // ============================================================================
// // Setup / Loop
// // ============================================================================
// void setup() {
//   Serial1.begin(115200, SERIAL_8E1);   // Your parity/baud style

//   // Bring up I2C
//   Wire1.begin();                       // Internal I2C
//   Wire4.begin();                       // External I2C (available)
//   delay(200);

//   // Init sensors
//   InitSensors();

//   // Init SD (maps pins + begin + open file + header if new)
//   if (!InitSD()) {
//     Serial1.println("SD init FAILED. Logging disabled.");
//   } else {
//     Serial1.println("SD init OK. Logging to sensors.csv");
//   }
// }

// void loop() {
//   // Read sensors frequently (100 ms)
//   ReadAccel();
//   ReadGyro();
//   ReadMag();
//   ReadTempLM75();
//   ReadBaroMS5611();

//   // Serial framed output (fast)
//   send_data();

//   // Log to SD every 5 seconds (non-blocking cadence)
//   const uint32_t now = millis();
//   if (now - lastLogMs >= LOG_INTERVAL_MS) {
//     lastLogMs = now;
//     LogToSD();
//   }

//   // Optional: verbose debug CSV every loop
//   // print_debug_csv();

//   delay(100); // Sensor polling/display rate
// }

// // ============================================================================
// // Init block
// // ============================================================================
// void InitSensors(void) {
//   // LSM6DS3 Accel
//   delay(100);
//   imu.InitAccel();
//   ACCEL_FLAG = true;
//   delay(50);

//   // LSM6DS3 Gyro
//   imu.InitGyro();
//   GYRO_FLAG = true;
//   delay(50);

//   // LIS3MDL Magnetometer
//   mag.Init();
//   MAG_FLAG = true;
//   delay(50);

//   // LM75A Temperature
//   tempLM75.Init();
//   TEMP_FLAG = true;
//   delay(50);

//   // MS5611 Barometer
//   baro.Init();
//   BARO_FLAG = true;
//   delay(50);
// }

// // ============================================================================
// // Read blocks (one per sensor, modular)
// // ============================================================================
// void ReadAccel(void) {
//   // Keep your axis-call style consistent with earlier code
//   Ax = imu.AZ();
//   Ay = imu.AY();
//   Az = imu.AX();
// }

// void ReadGyro(void) {
//   gx = imu.GX();
//   gy = imu.GY();
//   gz = imu.GZ();
// }

// void ReadMag(void) {
//   Mx = mag.MX();
//   My = mag.MY();
//   Mz = mag.MZ();
// }

// void ReadTempLM75(void) {
//   T_LM75  = tempLM75.GetTemperature();
//   T8_LM75 = tempLM75.GetTemperatureTimes8();
// }

// void ReadBaroMS5611(void) {
//   T_MS = baro.GetTemperature();  // °C
//   P_MS = baro.GetPressure();     // Library units (Pa or mbar — per your lib)
//   H_MS = baro.GetHeight();       // m
// }

// // ============================================================================
// // Framed compact sender (fixed field order; easy to parse)
// // ============================================================================
// void send_data(void) {
//   Serial1.print('*');

//   // Flags first (1/0) — consistent header
//   Serial1.print(ACCEL_FLAG ? '1' : '0'); Serial1.print(',');
//   Serial1.print(MAG_FLAG   ? '1' : '0'); Serial1.print(',');
//   Serial1.print(GYRO_FLAG  ? '1' : '0'); Serial1.print(',');
//   Serial1.print(TEMP_FLAG  ? '1' : '0'); Serial1.print(',');
//   Serial1.print(BARO_FLAG  ? '1' : '0'); Serial1.print(',');

//   // Accel
//   if (ACCEL_FLAG) { Serial1.print(Ax,3); Serial1.print(','); Serial1.print(Ay,3); Serial1.print(','); Serial1.print(Az,3); Serial1.print(','); }
//   else            { Serial1.print("0.000,0.000,0.000,"); }

//   // Mag
//   if (MAG_FLAG)   { Serial1.print(Mx,3); Serial1.print(','); Serial1.print(My,3); Serial1.print(','); Serial1.print(Mz,3); Serial1.print(','); }
//   else            { Serial1.print("0.000,0.000,0.000,"); }

//   // Gyro
//   if (GYRO_FLAG)  { Serial1.print(gx,3); Serial1.print(','); Serial1.print(gy,3); Serial1.print(','); Serial1.print(gz,3); Serial1.print(','); }
//   else            { Serial1.print("0.000,0.000,0.000,"); }

//   // LM75 (Temperature + Temperature*8)
//   if (TEMP_FLAG)  { Serial1.print(T_LM75,2); Serial1.print(','); Serial1.print(T8_LM75,2); Serial1.print(','); }
//   else            { Serial1.print("0.00,0.00,"); }

//   // MS5611 (Temp, Pressure, Height)
//   if (BARO_FLAG)  { Serial1.print(T_MS,2); Serial1.print(','); Serial1.print(P_MS,2); Serial1.print(','); Serial1.print(H_MS,2); }
//   else            { Serial1.print("0.00,0.00,0.00"); }

//   Serial1.println('#');
// }

// // ============================================================================
// // Optional CSV debug (human-readable, easy to paste into logs)
// // ============================================================================
// void print_debug_csv(void) {
//   Serial1.print("FLAGS: A=");
//   Serial1.print(ACCEL_FLAG ? "1" : "0");
//   Serial1.print(", M=");
//   Serial1.print(MAG_FLAG   ? "1" : "0");
//   Serial1.print(", G=");
//   Serial1.print(GYRO_FLAG  ? "1" : "0");
//   Serial1.print(", T=");
//   Serial1.print(TEMP_FLAG  ? "1" : "0");
//   Serial1.print(", B=");
//   Serial1.println(BARO_FLAG ? "1" : "0");

//   Serial1.print("ACCEL,"); Serial1.print(Ax); Serial1.print(','); Serial1.print(Ay); Serial1.print(','); Serial1.println(Az);
//   Serial1.print("MAG,");   Serial1.print(Mx); Serial1.print(','); Serial1.print(My); Serial1.print(','); Serial1.println(Mz);
//   Serial1.print("GYRO,");  Serial1.print(gx); Serial1.print(','); Serial1.print(gy); Serial1.print(','); Serial1.println(gz);
//   Serial1.print("LM75,");  Serial1.print(T_LM75); Serial1.print(','); Serial1.println(T8_LM75);
//   Serial1.print("MS5611,");Serial1.print(T_MS); Serial1.print(','); Serial1.print(P_MS); Serial1.print(','); Serial1.println(H_MS);
// }

// // ============================================================================
// // SD Card Helpers
// // ============================================================================
// bool InitSD(void) {
//   // Reconfigure SDIO pins (per your example)
//   SD.setDx(SD_D0, SD_D1, SD_D2, SD_D3);
//   SD.setCMD(SD_CMD);
//   SD.setCK(SD_CK);

//   // Wait for SD (simple blocking init to ensure mounted before open)
//   uint32_t t0 = millis();
//   while (!SD.begin()) {
//     if (millis() - t0 > 3000) { // 3s timeout to avoid permanent block
//       Serial1.println("SD.begin() timeout");
//       return false;
//     }
//   }

//   // Try opening/creating the CSV file and write header if new
//   if (!OpenLogIfNeeded()) {
//     Serial1.println("Failed to open sensors.csv");
//     return false;
//   }

//   return true;
// }

// bool FileStartsWithHeader(File &f) {
//   f.seek(0);
//   char buf[16] = {0};
//   int n = f.read(buf, 15);
//   f.seek(f.size()); // back to end for appending
//   return (n >= 12) && (strncmp(buf, "timestamp_ms", 12) == 0);
// }

// bool OpenLogIfNeeded(void) {
//   if (SensorData) return true;

//   // Try to open existing file for append
//   SensorData = SD.open(kLogFile, FILE_WRITE);
//   if (!SensorData) return false;

//   if (SensorData.size() == 0) {
//     // brand new file
//     WriteCSVHeaderIfNew(SensorData);
//     SensorData.flush();
//     return true;
//   }

//   // Existing file — check if it already has the header
//   if (FileStartsWithHeader(SensorData)) {
//     return true; // good, keep appending
//   }

//   // Missing header: rotate old file and create a fresh one
//   SensorData.close();
//   String bak = String("sensors_old.csv");
//   SD.remove(bak.c_str());          // ignore result
//   SD.rename(kLogFile, bak.c_str()); // keep old data safe

//   SensorData = SD.open(kLogFile, FILE_WRITE);
//   if (!SensorData) return false;
//   WriteCSVHeaderIfNew(SensorData);
//   SensorData.flush();
//   return true;
// }


// void WriteCSVHeaderIfNew(File &f) {
//   if (f.size() == 0) {
//     // Header row for easy spreadsheet import
//     f.println(
//       "timestamp_ms,"
//       "ACCEL_FLAG,MAG_FLAG,GYRO_FLAG,TEMP_FLAG,BARO_FLAG,"
//       "Ax,Ay,Az,"
//       "Mx,My,Mz,"
//       "gx,gy,gz,"
//       "T_LM75,T8_LM75,"
//       "T_MS,P_MS,H_MS"
//     );
//   }
// }

// void LogToSD(void) {
//   // Ensure card & file are available
//   if (!OpenLogIfNeeded()) {
//     Serial1.println("SD not ready / file open failed");
//     return;
//   }

//   // Timestamp (ms since boot). Replace with RTC time if available.
//   const uint32_t t_ms = millis();

//   // Write one CSV row (matching header order)
//   SensorData.print(t_ms); SensorData.print(',');

//   SensorData.print(ACCEL_FLAG ? 1 : 0); SensorData.print(',');
//   SensorData.print(MAG_FLAG   ? 1 : 0); SensorData.print(',');
//   SensorData.print(GYRO_FLAG  ? 1 : 0); SensorData.print(',');
//   SensorData.print(TEMP_FLAG  ? 1 : 0); SensorData.print(',');
//   SensorData.print(BARO_FLAG  ? 1 : 0); SensorData.print(',');

//   // Accel
//   SensorData.print(Ax, 3); SensorData.print(',');
//   SensorData.print(Ay, 3); SensorData.print(',');
//   SensorData.print(Az, 3); SensorData.print(',');

//   // Mag
//   SensorData.print(Mx, 3); SensorData.print(',');
//   SensorData.print(My, 3); SensorData.print(',');
//   SensorData.print(Mz, 3); SensorData.print(',');

//   // Gyro
//   SensorData.print(gx, 3); SensorData.print(',');
//   SensorData.print(gy, 3); SensorData.print(',');
//   SensorData.print(gz, 3); SensorData.print(',');

//   // LM75
//   SensorData.print(T_LM75, 2);  SensorData.print(',');
//   SensorData.print(T8_LM75, 2); SensorData.print(',');

//   // MS5611
//   SensorData.print(T_MS, 2); SensorData.print(',');
//   SensorData.print(P_MS, 2); SensorData.print(',');
//   SensorData.print(H_MS, 2);

//   SensorData.println();
//   SensorData.flush();  // ensure commit to card

//   // Optional: confirm to Serial
//   Serial1.println("SD: logged row.");
// }

// ============================================================================
// STM32 Custom Board: Internal Sensors + SD CSV Logging (NO LORA)
// UART:  Serial1(PA10, PA9)
// I2C:   Wire1(PB7,PB6) + Wire4(PD13,PD12)
// SDIO:  STM32SD with remapped pins (PC8..PC12, PD2) and detect pin PB8
//
// Sensors (I2C on Wire1):
//   - LSM6DS3 (Accel + Gyro) @ 0x6A
//   - LIS3MDL (Magnetometer) @ 0x1C (alt 0x1E)
//   - LM75A   (Temperature)  @ 0x4A
//   - MS5611  (Barometer)    @ 0x77
//
// CSV file: sensors.csv
// Columns:
//   timestamp_ms,
//   ACCEL_FLAG,MAG_FLAG,GYRO_FLAG,TEMP_FLAG,BARO_FLAG,
//   Ax,Ay,Az, Mx,My,Mz, gx,gy,gz, T_LM75,T8_LM75, T_MS,P_MS,H_MS
//
// Logging frequency: every 5000 ms (5 s)
// ============================================================================

// --- Libraries ---
#include <Arduino.h>
#include <LSM6DS3.h>   // Accel + Gyro
#include <LIS3MDL.h>   // Magnetometer
#include <LM75A.h>     // Temperature (ambient)
#include <MS5611.h>    // Barometer (temp, pressure, height)
#include <STM32SD.h>   // SD card (STM32)

// --- Namespace (as per your platform libs) ---
using namespace IntroStratLib;

// --- UART ---
HardwareSerial Serial1(PA10, PA9);  // Bluetooth / Telemetry

// --- I2C Buses ---
TwoWire Wire1(PB7, PB6);   // Internal sensors I2C
TwoWire Wire4(PD13, PD12); // External sensors I2C (reserved/available)

// --- Sensor Objects (all on Wire1 here) ---
LSM6DS3 imu(Wire1, 0x6A);      // Accel + Gyro (same chip)
LIS3MDL mag(Wire1, 0x1C);      // Magnetometer  (use 0x1E if strapped)
LM75A   tempLM75(Wire1, 0x4A); // Temperature sensor
MS5611  baro(Wire1, 0x77);     // Barometer

// --- Global Readings ---
float Ax = 0.0f, Ay = 0.0f, Az = 0.0f;         // Accel
float gx = 0.0f, gy = 0.0f, gz = 0.0f;         // Gyro
float Mx = 0.0f, My = 0.0f, Mz = 0.0f;         // Magnetometer
float T_LM75 = 0.0f, T8_LM75 = 0.0f;           // LM75 (°C and °C*8)
float T_MS = 0.0f, P_MS = 0.0f, H_MS = 0.0f;   // MS5611 (°C, Pa/mbar, m)

// --- Status Flags ---
bool ACCEL_FLAG = false;
bool GYRO_FLAG  = false;
bool MAG_FLAG   = false;
bool TEMP_FLAG  = false;
bool BARO_FLAG  = false;

// --- SD Card (pins + file) ---
#ifndef SD_DETECT_PIN
#define SD_DETECT_PIN PB8   // Detect pin
#endif

// SDIO remap pins (per your board)
#define SD_D0   PC8
#define SD_D1   PC9
#define SD_D2   PC10
#define SD_D3   PC11
#define SD_CMD  PD2
#define SD_CK   PC12

File SensorData;
const char* kLogFile = "sensors.csv";

// --- Logging cadence ---
const uint32_t LOG_INTERVAL_MS = 5000;  // 5 seconds
uint32_t lastLogMs = 0;

// --- Prototypes ---
void InitSensors(void);
void ReadAccel(void);
void ReadGyro(void);
void ReadMag(void);
void ReadTempLM75(void);
void ReadBaroMS5611(void);
void send_data(void);
void print_debug_csv(void);

// SD helpers
bool InitSD(void);
bool OpenLogIfNeeded(void);
bool FileStartsWithHeader(File &f);
void WriteCSVHeaderIfNew(File &f);
void LogToSD(void);

// --- NEW: rename replacement ---
bool copyFile(const char* src, const char* dst);

// ============================================================================
// Setup / Loop
// ============================================================================
void setup() {
  Serial1.begin(115200, SERIAL_8E1);   // Your parity/baud style

  // Bring up I2C
  Wire1.begin();                       // Internal I2C
  Wire4.begin();                       // External I2C (available)
  delay(200);

  // Init sensors
  InitSensors();

  // Init SD (maps pins + begin + open file + header if new)
  if (!InitSD()) {
    Serial1.println("SD init FAILED. Logging disabled.");
  } else {
    Serial1.println("SD init OK. Logging to sensors.csv");
  }
}

void loop() {
  // Read sensors frequently (100 ms)
  ReadAccel();
  ReadGyro();
  ReadMag();
  ReadTempLM75();
  ReadBaroMS5611();

  // Serial framed output (fast)
  send_data();

  // Log to SD every 5 seconds (non-blocking cadence)
  const uint32_t now = millis();
  if (now - lastLogMs >= LOG_INTERVAL_MS) {
    lastLogMs = now;
    LogToSD();
  }

  // Optional: verbose debug CSV every loop
  // print_debug_csv();

  delay(100); // Sensor polling/display rate
}

// ============================================================================
// Init block
// ============================================================================
void InitSensors(void) {
  // LSM6DS3 Accel
  delay(100);
  imu.InitAccel();
  ACCEL_FLAG = true;
  delay(50);

  // LSM6DS3 Gyro
  imu.InitGyro();
  GYRO_FLAG = true;
  delay(50);

  // LIS3MDL Magnetometer
  mag.Init();
  MAG_FLAG = true;
  delay(50);

  // LM75A Temperature
  tempLM75.Init();
  TEMP_FLAG = true;
  delay(50);

  // MS5611 Barometer
  baro.Init();
  BARO_FLAG = true;
  delay(50);
}

// ============================================================================
// Read blocks (one per sensor, modular)
// ============================================================================
void ReadAccel(void) {
  // Keep your axis-call style consistent with earlier code
  Ax = imu.AZ();
  Ay = imu.AY();
  Az = imu.AX();
}

void ReadGyro(void) {
  gx = imu.GX();
  gy = imu.GY();
  gz = imu.GZ();
}

void ReadMag(void) {
  Mx = mag.MX();
  My = mag.MY();
  Mz = mag.MZ();
}

void ReadTempLM75(void) {
  T_LM75  = tempLM75.GetTemperature();
  T8_LM75 = tempLM75.GetTemperatureTimes8();
}

void ReadBaroMS5611(void) {
  T_MS = baro.GetTemperature();  // °C
  P_MS = baro.GetPressure();     // Library units (Pa or mbar — match your lib)
  H_MS = baro.GetHeight();       // m
}

// ============================================================================
// Framed compact sender (fixed field order; easy to parse)
// ============================================================================
void send_data(void) {
  Serial1.print('*');

  // Flags first (1/0)
  Serial1.print(ACCEL_FLAG ? '1' : '0'); Serial1.print(',');
  Serial1.print(MAG_FLAG   ? '1' : '0'); Serial1.print(',');
  Serial1.print(GYRO_FLAG  ? '1' : '0'); Serial1.print(',');
  Serial1.print(TEMP_FLAG  ? '1' : '0'); Serial1.print(',');
  Serial1.print(BARO_FLAG  ? '1' : '0'); Serial1.print(',');

  // Accel
  if (ACCEL_FLAG) { Serial1.print(Ax,3); Serial1.print(','); Serial1.print(Ay,3); Serial1.print(','); Serial1.print(Az,3); Serial1.print(','); }
  else            { Serial1.print("0.000,0.000,0.000,"); }

  // Mag
  if (MAG_FLAG)   { Serial1.print(Mx,3); Serial1.print(','); Serial1.print(My,3); Serial1.print(','); Serial1.print(Mz,3); Serial1.print(','); }
  else            { Serial1.print("0.000,0.000,0.000,"); }

  // Gyro
  if (GYRO_FLAG)  { Serial1.print(gx,3); Serial1.print(','); Serial1.print(gy,3); Serial1.print(','); Serial1.print(gz,3); Serial1.print(','); }
  else            { Serial1.print("0.000,0.000,0.000,"); }

  // LM75 (Temperature + Temperature*8)
  if (TEMP_FLAG)  { Serial1.print(T_LM75,2); Serial1.print(','); Serial1.print(T8_LM75,2); Serial1.print(','); }
  else            { Serial1.print("0.00,0.00,"); }

  // MS5611 (Temp, Pressure, Height)
  if (BARO_FLAG)  { Serial1.print(T_MS,2); Serial1.print(','); Serial1.print(P_MS,2); Serial1.print(','); Serial1.print(H_MS,2); }
  else            { Serial1.print("0.00,0.00,0.00"); }

  Serial1.println('#');
}

// ============================================================================
// Optional CSV debug (human-readable, easy to paste into logs)
// ============================================================================
void print_debug_csv(void) {
  Serial1.print("FLAGS: A=");
  Serial1.print(ACCEL_FLAG ? "1" : "0");
  Serial1.print(", M=");
  Serial1.print(MAG_FLAG   ? "1" : "0");
  Serial1.print(", G=");
  Serial1.print(GYRO_FLAG  ? "1" : "0");
  Serial1.print(", T=");
  Serial1.print(TEMP_FLAG  ? "1" : "0");
  Serial1.print(", B=");
  Serial1.println(BARO_FLAG ? "1" : "0");

  Serial1.print("ACCEL,"); Serial1.print(Ax); Serial1.print(','); Serial1.print(Ay); Serial1.print(','); Serial1.println(Az);
  Serial1.print("MAG,");   Serial1.print(Mx); Serial1.print(','); Serial1.print(My); Serial1.print(','); Serial1.println(Mz);
  Serial1.print("GYRO,");  Serial1.print(gx); Serial1.print(','); Serial1.print(gy); Serial1.print(','); Serial1.println(gz);
  Serial1.print("LM75,");  Serial1.print(T_LM75); Serial1.print(','); Serial1.println(T8_LM75);
  Serial1.print("MS5611,");Serial1.print(T_MS); Serial1.print(','); Serial1.print(P_MS); Serial1.print(','); Serial1.println(H_MS);
}

// ============================================================================
// SD Card Helpers
// ============================================================================
bool InitSD(void) {
  // Reconfigure SDIO pins
  SD.setDx(SD_D0, SD_D1, SD_D2, SD_D3);
  SD.setCMD(SD_CMD);
  SD.setCK(SD_CK);

  // Wait for SD (simple blocking init to ensure mounted before open)
  uint32_t t0 = millis();
  while (!SD.begin()) {
    if (millis() - t0 > 3000) { // 3s timeout
      Serial1.println("SD.begin() timeout");
      return false;
    }
  }

  // Try opening/creating the CSV file and write header if new
  if (!OpenLogIfNeeded()) {
    Serial1.println("Failed to open sensors.csv");
    return false;
  }

  return true;
}

bool FileStartsWithHeader(File &f) {
  f.seek(0);
  char buf[16] = {0};
  int n = f.read(buf, 15);
  f.seek(f.size()); // back to end for appending
  return (n >= 12) && (strncmp(buf, "timestamp_ms", 12) == 0);
}

// --- NEW: copy-based rotation (STM32SD has no SD.rename) ---
bool copyFile(const char* src, const char* dst) {
  File in = SD.open(src, FILE_READ);
  if (!in) return false;

  SD.remove(dst); // best-effort
  File out = SD.open(dst, FILE_WRITE);
  if (!out) { in.close(); return false; }

  uint8_t buf[512];
  while (in.available()) {
    size_t n = in.read(buf, sizeof(buf));
    if (n == 0) break;
    size_t w = out.write(buf, n);
    if (w != n) { in.close(); out.close(); return false; }
  }
  out.flush();
  out.close();
  in.close();
  return true;
}

bool OpenLogIfNeeded(void) {
  if (SensorData) return true;

  // Try to open existing file for append
  SensorData = SD.open(kLogFile, FILE_WRITE);
  if (!SensorData) return false;

  if (SensorData.size() == 0) {
    // brand new file
    WriteCSVHeaderIfNew(SensorData);
    SensorData.flush();
    return true;
  }

  // Existing file — check if it already has the header
  if (FileStartsWithHeader(SensorData)) {
    return true; // good, keep appending
  }

  // Missing header: rotate old file and create a fresh one (no SD.rename in STM32SD)
  SensorData.close();
  const char* bak = "sensors_old.csv";
  if (!copyFile(kLogFile, bak)) {
    Serial1.println("WARN: could not copy old sensors.csv to sensors_old.csv");
  }
  // remove the original so we can create a fresh one with header
  SD.remove(kLogFile);

  SensorData = SD.open(kLogFile, FILE_WRITE);
  if (!SensorData) return false;
  WriteCSVHeaderIfNew(SensorData);
  SensorData.flush();
  return true;
}

void WriteCSVHeaderIfNew(File &f) {
  if (f.size() == 0) {
    // Header row for easy spreadsheet import
    f.println(
      "timestamp_ms,"
      "ACCEL_FLAG,MAG_FLAG,GYRO_FLAG,TEMP_FLAG,BARO_FLAG,"
      "Ax,Ay,Az,"
      "Mx,My,Mz,"
      "gx,gy,gz,"
      "T_LM75,T8_LM75,"
      "T_MS,P_MS,H_MS"
    );
  }
}

void LogToSD(void) {
  // Ensure card & file are available
  if (!OpenLogIfNeeded()) {
    Serial1.println("SD not ready / file open failed");
    return;
  }

  // Timestamp (ms since boot). Replace with RTC time if available.
  const uint32_t t_ms = millis();

  // Write one CSV row (matching header order)
  SensorData.print(t_ms); SensorData.print(',');

  SensorData.print(ACCEL_FLAG ? 1 : 0); SensorData.print(',');
  SensorData.print(MAG_FLAG   ? 1 : 0); SensorData.print(',');
  SensorData.print(GYRO_FLAG  ? 1 : 0); SensorData.print(',');
  SensorData.print(TEMP_FLAG  ? 1 : 0); SensorData.print(',');
  SensorData.print(BARO_FLAG  ? 1 : 0); SensorData.print(',');

  // Accel
  SensorData.print(Ax, 3); SensorData.print(',');
  SensorData.print(Ay, 3); SensorData.print(',');
  SensorData.print(Az, 3); SensorData.print(',');

  // Mag
  SensorData.print(Mx, 3); SensorData.print(',');
  SensorData.print(My, 3); SensorData.print(',');
  SensorData.print(Mz, 3); SensorData.print(',');

  // Gyro
  SensorData.print(gx, 3); SensorData.print(',');
  SensorData.print(gy, 3); SensorData.print(',');
  SensorData.print(gz, 3); SensorData.print(',');

  // LM75
  SensorData.print(T_LM75, 2);  SensorData.print(',');
  SensorData.print(T8_LM75, 2); SensorData.print(',');

  // MS5611
  SensorData.print(T_MS, 2); SensorData.print(',');
  SensorData.print(P_MS, 2); SensorData.print(',');
  SensorData.print(H_MS, 2);

  SensorData.println();
  SensorData.flush();  // ensure commit to card

  // Optional: confirm to Serial
  Serial1.println("SD: logged row.");
}
