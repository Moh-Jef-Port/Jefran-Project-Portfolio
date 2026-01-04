// //Connect the device power from the GPS module 

// #include <Wire.h>

// HardwareSerial Serial1(PA10, PA9);

// void setup() {
//   Wire.setSDA(PD13);
//   Wire.setSCL(PD12);
//   Serial1.begin(115200, SERIAL_8E1);
//   Wire.begin();
//   Serial1.println("\nI2C Scanner");
// }


// void loop() {
//   byte error, address;
//   int nDevices;

//   Serial1.println("Scanning...");

//   nDevices = 0;
//   for(address = 1; address < 127; address++) {
//     // The i2c_scanner uses the return value of
//     // the Write.endTransmisstion to see if
//     // a device did acknowledge to the address.

//     Wire.beginTransmission(address);
//     error = Wire.endTransmission();

//     if (error == 0) {
//       Serial1.print("I2C device found at address 0x");
//       if (address < 16)
//         Serial1.print("0");
//       Serial1.println(address, HEX);

//       nDevices++;
//     }
//     else if (error == 4) {
//       Serial1.print("Unknown error at address 0x");
//       if (address < 16)
//         Serial1.print("0");
//       Serial1.println(address, HEX);
//     }
//   }
//   if (nDevices == 0)
//     Serial1.println("No I2C devices found");
//   else
//     Serial1.println("done");

//   delay(5000);           // wait 5 seconds for next scan
// }


