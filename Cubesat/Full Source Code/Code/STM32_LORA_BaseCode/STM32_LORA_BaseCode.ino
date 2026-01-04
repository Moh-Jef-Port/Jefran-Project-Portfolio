HardwareSerial Serial1(PA10, PA9);
void setup(){ Serial1.begin(115200, SERIAL_8E1); }
void loop(){
  const char* msg = "HELLO_Team_UAE_For_the_win\r\n";
  Serial1.write((const uint8_t*)msg, 30);
  // wait for radio ACK (0x06) before sending again
  uint32_t t0 = millis();
  while (millis()-t0 < 1200) { if (Serial1.available() && Serial1.read()==0x06) break; }
  delay(500);





