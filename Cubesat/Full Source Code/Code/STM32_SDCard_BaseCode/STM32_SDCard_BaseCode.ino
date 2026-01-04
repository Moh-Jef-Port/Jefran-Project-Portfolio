// //SD Card Base Code (Uploading Data from sensors to SD Card )
// //Open Fatfs and STM32SD codes to check for how to write in libraries

// //using namespace IntroStratLib;  // Telling the Arduino to use the names from the Library 
// HardwareSerial Serial1(PA10, PA9); //Initialise the pins for UART serial communication Serial1: Bluetooth (PA10, PA9) = SERIAL 1

// //SD Card Library and Pin initialisation process --> Detecting the SD Card
// #include <STM32SD.h> //include STM32SD library 
// #ifndef SD_DETECT_PIN //To detect to which pins SD card is connected to
// #define SD_DETECT_PIN PB8 //its connected to PB8 
// #endif 

// File SensorData; //Creation of the file (object) of datatype file 

// void setup() {
  
//   Serial1.begin(115200, SERIAL_8E1); //Begin Serial1 Communication with baud rate of 115200 and 8 bit Even Parity

//   //Reconfigure the standard interface pins and assign them to the SD card
//   SD.setDx(PC8,PC9,PC10,PC11);
//   SD.setCMD(PD2);
//   SD.setCK(PC12);
//   //End of Reconfiguration

// }

// void loop() {

//   while (!SD.begin()); //initialise data exchange with the SD Card
  
//   //Perform File Handling Operations 
//   SensorData = SD.open("sensordata.txt", FILE_WRITE); //Opening the file for Write mode to write to it (Happens at the first free line)
//   // Open with FILE_READ to read the file 

//   //Check if the file has been opened before writing to it 
//   if (!SensorData){
//     Serial1.print("File not opened, error occured");
//   }

//   //Writing String format data to a succesfully opened file using PrintIn() Method, use flush() function to check for succefull data writing
//   String dataString = "hello";
//   if (SensorData) {
//     SensorData.println(dataString);
//     SensorData.flush();
//     Serial1.print("File writing Complete");
//   }

// }
//__________________________________________________________________________________________________________

//Code to write data into SD card from sensor values (Example Taken: LSM6DS3 - Accelerometer)

// ===== LSM6DS3 + SD card logger (STM32, SDIO remap pins) =====


#include <LSM6DS3.h>
#include <STM32SD.h>
using namespace IntroStratLib;

HardwareSerial Serial1(PA10, PA9);
TwoWire Wire1(PB7, PB6);
LSM6DS3 imu(Wire1, 0x6A);

#ifndef SD_DETECT_PIN
#define SD_DETECT_PIN PB8
#endif

File SensorData;
float Ax, Ay, Az = 0.0f;

void setup() {
  Serial1.begin(115200, SERIAL_8E1);

  // SD pin mapping
  SD.setDx(PC8,PC9,PC10,PC11);
  SD.setCMD(PD2);
  SD.setCK(PC12);

  Wire1.begin();
  delay(100);
  imu.InitAccel();
  delay(100);

  // Initialize SD
  while (!SD.begin());

  // Create/open file
  SensorData = SD.open("sensordata.csv", FILE_WRITE);
  if (!SensorData) {
    Serial1.println("File not opened, error occurred!");
    while (1); // stop if file can't open
  }

  // Add header if file is new
  if (SensorData.size() == 0) {
    SensorData.println("Ax,Ay,Az");
    SensorData.flush();
  }
}

void loop() {
  Ax = imu.AX();
  Ay = imu.AY();
  Az = imu.AZ();

  if (SensorData) {
    SensorData.print(Ax); SensorData.print(",");
    SensorData.print(Ay); SensorData.print(",");
    SensorData.println(Az);
    SensorData.flush();

    Serial1.print("Logged: ");
    Serial1.print(Ax); Serial1.print(", ");
    Serial1.print(Ay); Serial1.print(", ");
    Serial1.println(Az);
  }

  delay(500);
}

