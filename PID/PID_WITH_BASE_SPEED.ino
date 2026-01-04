// === IR sensor pins ===
#define IR_SENSOR_LEFT   2
#define IR_SENSOR_RIGHT  4

// === Motor control pins ===
int enableLeftMotor  = 5;
int leftMotorPin1    = 10;
int leftMotorPin2    = 11;
int enableRightMotor = 6;
int rightMotorPin1   = 12;
int rightMotorPin2   = 13;

// === Ultrasonic sensor pins ===
const int trigPin = 8;
const int echoPin = 9;

// === PID Constants ===
float Kp = 100, Ki = 0, Kd = 35;
float error = 0, lastError = 0, integral = 0;

// === Speeds ===
int baseSpeed     = 115;
int minSpeed      = 50;
int straightSpeed = 115;
int turnSpeed     = 70;

// === Obstacle escape speeds ===
int escapeTurnSpeed   = 150;
int escapeDriveSpeed  = 170;

// === Obstacle threshold ===
const float obstacleThreshold = 20.0;

// === Resume flag ===
bool resumePID = true;
int lastTurnDirection = 0;

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

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  rotateMotor(0, 0);
}

void loop() {
  float distance = readSonar();

  // === Obstacle detected ===
  if (distance > 0 && distance < obstacleThreshold) {
    resumePID = false;  // Block PID until rerouting finishes
    escapeObstacle();
    return;
  }

  // === If not ready to resume PID, wait ===
  if (!resumePID) return;

  // === PID Starts ===
  int leftIR = digitalRead(IR_SENSOR_LEFT);
  int rightIR = digitalRead(IR_SENSOR_RIGHT);

  bool leftOnLine = (leftIR == HIGH);
  bool rightOnLine = (rightIR == HIGH);

  if (leftOnLine && !rightOnLine) {
    error = 1;
    lastTurnDirection = 1;
  }
  else if (!leftOnLine && rightOnLine) {
    error = -1;
    lastTurnDirection = -1;
  }
  else if (!leftOnLine && !rightOnLine) {
    error = 0;
    lastTurnDirection = 0;
  }
  else {
    if (lastTurnDirection == 1)
      rotateMotor(turnSpeed, -turnSpeed);
    else if (lastTurnDirection == -1)
      rotateMotor(-turnSpeed, turnSpeed);
    else
      rotateMotor(0, 0);
    delay(10);
    return;
  }

  baseSpeed = (error == 0) ? straightSpeed : turnSpeed;
  integral += error;
  float derivative = error - lastError;
  float correction = Kp * error + Ki * integral + Kd * derivative;
  lastError = error;

  int leftSpeed = constrain(baseSpeed + correction, minSpeed, 255);
  int rightSpeed = constrain(baseSpeed - correction, minSpeed, 255);
  rotateMotor(leftSpeed, rightSpeed);
  delay(10);
}

// === Motor control ===
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

// === Sonar read ===
float readSonar() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return -1;
  return duration / 58.0;
}

// === Escape + Resume Control ===
void escapeObstacle() {
  // Step 1: Hard right turn
  rotateMotor(escapeTurnSpeed, -escapeTurnSpeed);
  delay(650);

  // Step 2: Drive forward full power until black line
  while (
    digitalRead(IR_SENSOR_LEFT) != HIGH && 
    digitalRead(IR_SENSOR_RIGHT) != HIGH
  ) {
    rotateMotor(escapeDriveSpeed, escapeDriveSpeed);
    delay(10);
  }

  // Step 3: Found line → stop + enable PID
  rotateMotor(0, 0);
  delay(100);
  resumePID = true;
}

