//Barometer (Temp, Pressure, Altitude) Base Code (MS5611)

#include <MS5611.h> //Include the library for the Barometer

using namespace IntroStratLib;  // Telling the Arduino to use the names from the Library 

HardwareSerial Serial1(PA10, PA9); //Initialise the pins for UART serial communication Serial1: Bluetooth (PA10, PA9) = SERIAL 1

TwoWire Wire1(PB7,PB6); //initialise The I2C pins and start the data and clock operations 

MS5611 barometer(Wire1, 0x77); //Create an object named "barometer" in the initialised I2C line, and specify it's address for communication for STM32 master 

float Temp, Pressure, Height = 0.0f; //Create Variables to receive barometer's 3 types of Data 

void setup() {

  Serial1.begin(115200, SERIAL_8E1); //Begin Serial1 Communication with baud rate of 115200 and 8 bit Even Parity

  Wire1.begin(); //Start operations in Wire1 I2C channel 

//barometer Block: To initialise and Synchronise barometer Data operations
  delay(100); 
  barometer.Init();
  delay(100);

}

void loop() {
  
  Temp = barometer.GetTemperature(); //Store barometer data of Temperature into variable Temp
  Pressure = barometer.GetPressure(); //Store barometer data of Pressure into variable Pressure
  Height = barometer.GetHeight(); //Store barometer data of Height into variable Height

  // Code to print all the gyro Values in Serial Monitor 

  Serial1.print(Temp); //Print Temp Values 
  Serial1.print(", "); //Print Value Spacer Comma 
  Serial1.print(Pressure); //Print Pressure Values 
  Serial1.print(", "); //Print Value Spacer Comma 
  Serial1.print(Height); //Print Height Values 
  Serial1.print(", "); //Print Value Spacer Comma 

  delay(500); //Delay 100ms so the values are not printed often and spammed  

}
