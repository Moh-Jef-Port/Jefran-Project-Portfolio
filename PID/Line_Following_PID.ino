// === Function Prototype ===
float readUltrasonicDistance();

// === IR sensor pins ===
#define IR_SENSOR_LEFT 2
#define IR_SENSOR_RIGHT 4

// === Motor control pins ===
int enableLeftMotor = 5;
int leftMotorPin1 = 10;
int leftMotorPin2 = 11;

int enableRightMotor = 6;
int rightMotorPin1 = 12;
int rightMotorPin2 = 13;

// === Ultrasonic sensor pins ===
#define TRIG_PIN 8
#define ECHO_PIN 9

// === PID tuning values ===
float Kp = 100;
float Ki = 0;
float Kd = 35;

float error = 0, lastError = 0, integral = 0;

// === Speed values ===
int baseSpeed = 115;
int minSpeed = 50;
int straightSpeed = 115;
int turnSpeed = 70;

// Last turn direction: 1 = right, -1 = left, 0 = no turn yet
int lastTurnDirection = 0;

// For detecting line loss duration
unsigned long lineLostStart = 0;
bool lineLost = false;

bool resumePID = true;

void setup() {
  TCCR0B = TCCR0B & B11111000 | B00000010;

  pinMode(enableLeftMotor, OUTPUT);
  pinMode(leftMotorPin1, OUTPUT);
  pinMode(leftMotorPin2, OUTPUT);

  pinMode(enableRightMotor, OUTPUT);
  pinMode(rightMotorPin1, OUTPUT);
  pinMode(rightMotorPin2, OUTPUT);

  pinMode(IR_SENSOR_LEFT, INPUT);
  pinMode(IR_SENSOR_RIGHT, INPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  rotateMotor(0, 0);
}

void loop() {
  float distance = readUltrasonicDistance();
  if (distance > 0 && distance < 15) {
    rotateMotor(0, 0);
    delay(100);
    resumePID = false;
    perform180AndSearch();
    resumePID = true;
    return;
  }

  if (!resumePID) return;

  int leftIR = digitalRead(IR_SENSOR_LEFT);
  int rightIR = digitalRead(IR_SENSOR_RIGHT);

  bool leftOnLine = (leftIR == HIGH);
  bool rightOnLine = (rightIR == HIGH);

  if (leftOnLine && rightOnLine) {
    if (!lineLost) {
      lineLost = true;
      lineLostStart = millis();
    } else if (millis() - lineLostStart > 1500) {
      rotateMotor(0, 0);
      delay(100);
      resumePID = false;
      perform180AndSearch();
      resumePID = true;
      lineLost = false;
      return;
    }
  } else {
    lineLost = false;
  }

  if (leftOnLine && !rightOnLine) {
    error = 1;
    lastTurnDirection = 1;
  } else if (!leftOnLine && rightOnLine) {
    error = -1;
    lastTurnDirection = -1;
  } else if (!leftOnLine && !rightOnLine) {
    error = 0;
    lastTurnDirection = 0;
  } else {
    if (lastTurnDirection == 1) {
      rotateMotor(turnSpeed, -turnSpeed);
    } else if (lastTurnDirection == -1) {
      rotateMotor(-turnSpeed, turnSpeed);
    } else {
      rotateMotor(0, 0);
    }
    delay(10);
    return;
  }

  baseSpeed = (error == 0) ? straightSpeed : turnSpeed;

  integral += error;
  float derivative = error - lastError;
  float correction = (Kp * error) + (Ki * integral) + (Kd * derivative);
  lastError = error;

  int leftSpeed = baseSpeed + correction;
  int rightSpeed = baseSpeed - correction;

  leftSpeed = constrain(leftSpeed, minSpeed, 255);
  rightSpeed = constrain(rightSpeed, minSpeed, 255);

  rotateMotor(leftSpeed, rightSpeed);
  delay(10);
}

void rotateMotor(int leftSpeed, int rightSpeed) {
  if (leftSpeed >= 0) {
    digitalWrite(leftMotorPin1, HIGH);
    digitalWrite(leftMotorPin2, LOW);
  } else {
    digitalWrite(leftMotorPin1, LOW);
    digitalWrite(leftMotorPin2, HIGH);
    leftSpeed = -leftSpeed;
  }
  analogWrite(enableLeftMotor, constrain(leftSpeed, 0, 255));

  if (rightSpeed >= 0) {
    digitalWrite(rightMotorPin1, HIGH);
    digitalWrite(rightMotorPin2, LOW);
  } else {
    digitalWrite(rightMotorPin1, LOW);
    digitalWrite(rightMotorPin2, HIGH);
    rightSpeed = -rightSpeed;
  }
  analogWrite(enableRightMotor, constrain(rightSpeed, 0, 255));
}

void perform180AndSearch() {
  int spinSpeed = 200;
  int moderateSearchSpeed = 130;

  // Step 1: Spin 180 degrees
  rotateMotor(spinSpeed, -spinSpeed);
  delay(900);
  rotateMotor(0, 0);
  delay(100);

  // Step 2: Drive forward and check IR continuously
  unsigned long timeout = 4000;
  unsigned long startTime = millis();

  while (millis() - startTime < timeout) {
    int leftIR = digitalRead(IR_SENSOR_LEFT);
    int rightIR = digitalRead(IR_SENSOR_RIGHT);

    if (leftIR == HIGH || rightIR == HIGH) {
      break;
    }

    // Keep driving forward
    rotateMotor(moderateSearchSpeed, moderateSearchSpeed);
    delay(10);
  }

  // Stop motors and prep PID
  rotateMotor(0, 0);
  delay(100);
  lastError = 0;
  integral = 0;
}
    delay(10);
  }

  rotateMotor(0, 0);
  delay(100);
  lastError = 0;
  integral = 0;
}

float readUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return 999;
  return duration * 0.034 / 2;
}
