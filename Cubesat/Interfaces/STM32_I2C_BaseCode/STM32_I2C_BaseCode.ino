//Guide to Work with I2C and internal sensor libraries 

TwoWire Wire1(PB7, PB6); //Creating a Class object for Communication lines of SDA/SCL 
TwoWire Wire4(PD13,PD12); //Second I2C SDA/SCL Line for communication 

LSM6DS3 gyroscope(Wire1, 0x6A); //Create an object named "gyroscope" in the initialised I2C line, and specify it's address for communication for STM32 master 






void setup() {
  Wire1.begin(); //Begin I2C communication lines

}

void loop() {
  // put your main code here, to run repeatedly:

}


//Data Exchange with peripheral devices
//Four Inbuilt sensors 

LSM6DS3.h //0x6A
LIS3MDL.h //0x1C
MS5611.h //0x77
LM75A.h //0x4A


//If external sensors are attached to the line, use the I2C scanner code and run the code to find their addresses and then code
