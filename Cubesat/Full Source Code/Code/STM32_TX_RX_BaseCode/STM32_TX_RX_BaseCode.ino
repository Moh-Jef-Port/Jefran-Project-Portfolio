using namespace IntroStratLib;

HardwareSerial Serial1(PA10, PA9);
HardwareSerial Serial3(PD9, PD8);

void setup() {
  Serial1.begin(115200, SERIAL_8E1);
  Serial3.begin(9600);
}

void loop() {
  if(Serial3.available()){
    Serial1.write(Serial3.read());
  }
}