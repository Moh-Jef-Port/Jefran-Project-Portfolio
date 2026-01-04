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

// === PID tuning values ===
float Kp = 100;
float Ki = 0;
float Kd = 35;

float error = 0, lastError = 0, integral = 0;

// === Speed values ===
int baseSpeed = 115;
int minSpeed = 50;
int straightSpeed = 115;
int turnSpeed =70;

// Last turn direction: 1 = right, -1 = left, 0 = no turn yet
int lastTurnDirection = 0;

void setup() {
  // Increase PWM frequency on D5/D6
  TCCR0B = TCCR0B & B11111000 | B00000010;

  pinMode(enableLeftMotor, OUTPUT);
  pinMode(leftMotorPin1, OUTPUT);
  pinMode(leftMotorPin2, OUTPUT);

  pinMode(enableRightMotor, OUTPUT);
  pinMode(rightMotorPin1, OUTPUT);
  pinMode(rightMotorPin2, OUTPUT);

  pinMode(IR_SENSOR_LEFT, INPUT);
  pinMode(IR_SENSOR_RIGHT, INPUT);

  rotateMotor(0, 0);
}

void loop() {
  int leftIR = digitalRead(IR_SENSOR_LEFT);
  int rightIR = digitalRead(IR_SENSOR_RIGHT);

  bool leftOnLine = (leftIR == HIGH);
  bool rightOnLine = (rightIR == HIGH);

  if (leftOnLine && !rightOnLine) {
    error = 1;
    lastTurnDirection = 1; // last turn right
  }
  else if (!leftOnLine && rightOnLine) {
    error = -1;
    lastTurnDirection = -1; // last turn left
  }
  else if (!leftOnLine && !rightOnLine) {
    // On line center
    error = 0;
    lastTurnDirection = 0;
  }
  else {
    // Both sensors on line (lost or intersection)
    // Keep turning HARD in last known direction to find line again
    if (lastTurnDirection == 1) {
      // Hard right turn: left motor forward full speed, right motor backward full speed
      rotateMotor(turnSpeed, -turnSpeed);
    }
    else if (lastTurnDirection == -1) {
      // Hard left turn
      rotateMotor(-turnSpeed, turnSpeed);
    }
    else {
      // No last turn direction - stop motors
      rotateMotor(0, 0);
    }
    delay(10);
    return; // skip PID control while recovering
  }

  // Adjust speed depending on error (turning or straight)
  baseSpeed = (error == 0) ? straightSpeed : turnSpeed;

  // PID calculations
  integral += error;
  float derivative = error - lastError;
  float correction = (Kp * error) + (Ki * integral) + (Kd * derivative);
  lastError = error;

  // Calculate motor speeds
  int leftSpeed = baseSpeed + correction;
  int rightSpeed = baseSpeed - correction;

  // Constrain motor speeds
  leftSpeed = constrain(leftSpeed, minSpeed, 255);
  rightSpeed = constrain(rightSpeed, minSpeed, 255);

  rotateMotor(leftSpeed, rightSpeed);

  delay(10);
}

void rotateMotor(int leftSpeed, int rightSpeed) {
  // Left motor forward or backward based on sign of leftSpeed
  if (leftSpeed >= 0) {
    digitalWrite(leftMotorPin1, HIGH);
    digitalWrite(leftMotorPin2, LOW);
  } else {
    digitalWrite(leftMotorPin1, LOW);
    digitalWrite(leftMotorPin2, HIGH);
    leftSpeed = -leftSpeed;  // make positive for PWM
  }
  analogWrite(enableLeftMotor, constrain(leftSpeed, 0, 255));

  // Right motor forward or backward based on sign of rightSpeed
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
