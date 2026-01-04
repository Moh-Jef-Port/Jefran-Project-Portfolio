#include <LSM6DS3.h> //Include the library for the Gyro 

using namespace IntroStratLib;  // Telling the Arduino to use the names from the Library 

HardwareSerial Serial1(PA10, PA9); //Initialise the pins for UART serial communication Serial1: Bluetooth (PA10, PA9) = SERIAL 1

TwoWire Wire1(PB7,PB6); //initialise The I2C pins and start the data and clock operations 

LSM6DS3 gyroscope(Wire1, 0x6A); //Create an object named "gyroscope" in the initialised I2C line, and specify it's address for communication for STM32 master 

float gx, gy, gz = 0.0f; //Create Variables to receive Gyro's 3 Axis Data 

void setup() {

serial1.begin(115200, SERIAL_8E1); //Begin Serial1 Communication with baud rate of 115200 and 8 bit Even Parity

Wire1.begin(); //Start operations in Wire1 I2C channel 

//Gyro Block: To initialise and Synchronise Gyro Data operations
delay(100);
gyroscope.InitGyro();
delay(100);

}

void loop() {

  gx = gyroscope.GX(); //Store Gyro data of axis X into variable gx 
  gy = gyroscope.GY(); //Store Gyro data of axis Y into variable gy
  gz = gyroscope.GZ(); //Store Gyro data of axis Z into variable gz

  //Raw Values 
  //gx = gyroscope.RawGX(); //Store Gyro data of axis X into variable gx 
  //gy = gyroscope.RawGy(); //Store Gyro data of axis Y into variable gy
  //gz = gyroscope.RawGz(); //Store Gyro data of axis Z into variable gz


// Code to print all the gyro Values in Serial Monitor 

  Serial1.print(gx); //Print Gx Values 
  Serial1.print(", "); //Print Value Spacer Comma 
  Serial1.print(gy); //Print Gy Values 
  Serial1.print(", "); //Print Value Spacer Comma 
  Serial1.print(gz); //Print Gz Values 

  delay(100); //Delay 100ms so the values are not printed often and spammed  
}
