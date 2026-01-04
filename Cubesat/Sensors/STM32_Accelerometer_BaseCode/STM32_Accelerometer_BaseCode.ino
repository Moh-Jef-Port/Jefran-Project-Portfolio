//Accelerometer Base Code Same Module as Gyro (LSM6DS3)

#include <LSM6DS3.h> //Include the library for the Gyro 

using namespace IntroStratLib;  // Telling the Arduino to use the names from the Library 

HardwareSerial Serial1(PA10, PA9); //Initialise the pins for UART serial communication Serial1: Bluetooth (PA10, PA9) = SERIAL 1

TwoWire Wire1(PB7,PB6); //initialise The I2C pins and start the data and clock operations 

LSM6DS3 gyroscope(Wire1, 0x6A); //Create an object named "gyroscope" in the initialised I2C line, and specify it's address for communication for STM32 master 

float Ax, Ay, Az = 0.0f; //Create Variables to receive Gyro's 3 Axis Data 

void setup() {

Serial1.begin(115200, SERIAL_8E1); //Begin Serial1 Communication with baud rate of 115200 and 8 bit Even Parity

Wire1.begin(); //Start operations in Wire1 I2C channel 

//Gyro Block: To initialise and Synchronise Gyro Data operations
delay(100);
gyroscope.InitAccel();
delay(100);

}

void loop() {

  Ax = gyroscope.AZ(); //Store Gyro data of axis X into variable Ax 
  Ay = gyroscope.AY(); //Store Gyro data of axis Y into variable Ay
  Az = gyroscope.AX(); //Store Gyro data of axis Z into variable Az

  //Raw Values 
  //Ax = gyroscope.RawAX(); //Store Gyro data of axis X into variable Ax 
  //Ay = gyroscope.RawAY(); //Store Gyro data of axis Y into variable Ay
  //Az = gyroscope.RawAZ(); //Store Gyro data of axis Z into variable Az


// Code to print all the gyro Values in Serial Monitor 

  Serial1.print(Ax); //Print Ax Values 
  Serial1.print(", "); //Print Value Spacer Comma 
  Serial1.print(Ay); //Print Ay Values 
  Serial1.print(", "); //Print Value Spacer Comma 
  Serial1.print(Az); //Print Az Values 
  Serial1.print(", "); //Print Value Spacer Comma 

  delay(500); //Delay 100ms so the values are not printed often and spammed  
}


//The Gyro uses data type: int16_t. (for serial monitor receiveing purposes)


