//Temperature Sensor Base code (LM75AD)

#include <LM75A.h> //Include the library for the Temperature Sensor

using namespace IntroStratLib;  // Telling the Arduino to use the names from the Library 

HardwareSerial Serial1(PA10, PA9); //Initialise the pins for UART serial communication Serial1: Bluetooth (PA10, PA9) = SERIAL 1

TwoWire Wire1(PB7,PB6); //initialise The I2C pins and start the data and clock operations 

LM75A TemperatureSensor(Wire1, 0x4A); //Create an object named "Temperature Sensor" in the initialised I2C line, and specify it's address for communication for STM32 master 

float Temperature,Times8Temp = 0.0f; //Create Variables to receive Temperature Sensor's 3 types of Data 

void setup() {

  Serial1.begin(115200, SERIAL_8E1); //Begin Serial1 Communication with baud rate of 115200 and 8 bit Even Parity

  Wire1.begin(); //Start operations in Wire1 I2C channel 

//Temperature Sensor Block: To initialise and Synchronise Temperature Sensor Data operations
  delay(100); 
  TemperatureSensor.Init();
  delay(100);

}

void loop() {
  
  Temperature = TemperatureSensor.GetTemperature(); //Store Temperature Sensor data of Temperature into variable Temperature
  Times8Temp = TemperatureSensor.GetTemperatureTimes8(); //Store Temperature Sensor Temp*8 data of Temperature into variable Temperature
  // Code to print all the Temperature Sensor Values in Serial Monitor 

  Serial1.print(Temperature); //Print Temp Values 
  Serial1.print(", "); //Print Value Spacer Comma 
  Serial1.print(Times8Temp); //Print Temp*8 Values (To save spaced, can be stored in 2 bytes and later decoded by dividing by 8)
  Serial1.print(", "); //Print Value Spacer Comma 

  delay(500); //Delay 100ms so the values are not printed often and spammed  

}
