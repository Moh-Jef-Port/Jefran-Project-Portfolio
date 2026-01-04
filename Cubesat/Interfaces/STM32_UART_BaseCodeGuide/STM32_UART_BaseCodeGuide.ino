// UART Communication Format
// Format: HardwareSerial MySerial(RX_Pin, TX_Pin);

//Available UART Communication Pins in the Flight controller Board
HardwareSerial Serial1(PA10, PA9);
HardwareSerial Serial3(PD9, PD8);
HardwareSerial Serial5(PB12, PB13);
HardwareSerial Serial8(PE0, PE1);

void setup() {

// How to Begin Communication in Serial Monitor 

serial1.begin(115200, SERIAL_8E1); //Begin Serial1 Communication with baud rate of 115200 and 8 bit Even Parity

Void loop(){

// How to Print Values in Serial Monitor
  Serial1.print(gx); //Print Gx Values 
  Serial1.print(", "); //Print Value Spacer Comma 
  Serial1.print(gy); //Print Gy Values 
  Serial1.print(", "); //Print Value Spacer Comma 
  Serial1.print(gz); //Print Gz Values 

//Ways of Data Transmission

//1. Printing in Serial Monitor: Serial1.print("Hello World"); --> Passing a string 

//2. Pass a Byte to a specific device in the I2C line: Serial1.write(Device Address);

//Example send the sensor the byte to open the servo: used as a signal

if (pressure > threshold) {
    Serial1.write(0x14); // Send the 'open' signal to the servo controller
}

//End of Ways of Data Transmission

//Recieving Data: Using Available() function to check for data recieval

if (Serial1.available() > 0) {
  uint8_t data = Serial1.read() //read 1 byte from the incoming data 
}
//Later do something logical using the data 




}