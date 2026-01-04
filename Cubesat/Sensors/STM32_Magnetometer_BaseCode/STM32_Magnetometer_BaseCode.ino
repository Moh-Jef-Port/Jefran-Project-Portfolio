//Magnetometer Base code (LIS3MDL)

#include <LIS3MDL.h> //Include the library for the Magnetometer

using namespace IntroStratLib;  // Telling the Arduino to use the names from the Library 

HardwareSerial Serial1(PA10, PA9); //Initialise the pins for UART serial communication Serial1: Bluetooth (PA10, PA9) = SERIAL 1

TwoWire Wire1(PB7,PB6); //initialise The I2C pins and start the data and clock operations 

LIS3MDL magnetometer(Wire1, 0x1C); //Create an object named "Magnetometer" in the initialised I2C line, and specify it's address for communication for STM32 master 

float Mx, My, Mz = 0.0f; //Create Variables to receive Magnetometer's 3 Axis Data 

void setup() {

Serial1.begin(115200, SERIAL_8E1); //Begin Serial1 Communication with baud rate of 115200 and 8 bit Even Parity

Wire1.begin(); //Start operations in Wire1 I2C channel 

//Magnetometer Block: To initialise and Synchronise Magnetometer Data operations
delay(100);
magnetometer.Init();
delay(100);

}

void loop() {

  Mx = magnetometer.Mx(); //Store Magnetometer data of axis X into variable Mx 
  My = magnetometer.My(); //Store Magnetometer data of axis Y into variable My
  Mz = magnetometer.Mz(); //Store Magnetometer data of axis Z into variable Mz

  //Raw Values 
  //Mx = magnetometer.RawMx(); //Store Magnetometer data of axis X into variable Mx 
  //My = magnetometer.RawMy(); //Store Magnetometer data of axis Y into variable My
  //Mz = magnetometer.RawMz(); //Store Magnetometer data of axis Z into variable Mz

// Code to print all the gyro Values in Serial Monitor 

  Serial1.print(Mx); //Print Mx Values 
  Serial1.print(", "); //Print Value Spacer Comma 
  Serial1.print(My); //Print My Values 
  Serial1.print(", "); //Print Value Spacer Comma 
  Serial1.print(Mz); //Print Mz Values 

  delay(100); //Delay 100ms so the values are not printed often and spammed  
}


//The Magnetometer uses data type: int16_t. (for serial monitor receiveing purposes)