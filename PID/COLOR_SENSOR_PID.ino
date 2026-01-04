#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>

// Pin definitions for ESP32X
#define MOTOR_LEFT_PWM 14
#define MOTOR_LEFT_DIR 15
#define MOTOR_RIGHT_PWM 13
#define MOTOR_RIGHT_DIR 12

#define IR_LEFT 2
#define IR_RIGHT 4

#define TRIG_PIN 1
#define ECHO_PIN 3

#define SERVO_PIN 16

// Color sensor pins (TCS3200/TCS230)
#define COLOR_S0 5
#define COLOR_S1 18
#define COLOR_S2 19
#define COLOR_S3 21
#define COLOR_OUT 22

// Servo variables
int servoFreq = 50;
int currentServoAngle = 90; // Track current servo position

// Traffic light colors
enum TrafficLightColor
{
    NONE = 0,
    RED = 1,
    BLUE = 2, // Changed from YELLOW to BLUE
    GREEN = 3
};

// Robot states
enum RobotState
{
    COLOR_TEST_MODE, // New test mode
    LINE_FOLLOWING,
    OBSTACLE_AVOIDANCE,
    TRAFFIC_LIGHT_STOP,
    TRAFFIC_LIGHT_SLOW
};

RobotState currentState = COLOR_TEST_MODE; // Start in test mode

// Motor speeds
const int NORMAL_SPEED = 150;
const int SLOW_SPEED = 80;
const int TURN_SPEED = 100;

// Distance threshold for obstacle detection (cm)
const int OBSTACLE_THRESHOLD = 15;

// Color sensor variables
unsigned long redFrequency = 0;
unsigned long greenFrequency = 0;
unsigned long blueFrequency = 0;

// Timing variables
unsigned long lastColorCheck = 0;
unsigned long colorCheckInterval = 300; // Check color every 300ms for faster detection

// Updated color calibration thresholds based on your readings
const unsigned long RED_DETECT_THRESHOLD = 40;   // Red frequency should be < 40
const unsigned long GREEN_DETECT_THRESHOLD = 55; // Green frequency should be < 55
const unsigned long BLUE_DETECT_THRESHOLD = 35;  // Blue frequency should be < 35

// Minimum difference between colors for reliable detection
const unsigned long COLOR_DIFFERENCE_MIN = 15;

// Red detection tracking
bool lastRedState = false;
unsigned long redDetectionCount = 0;

void setup()
{
    // Initialize Serial Monitor at 115200 baud rate
    Serial.begin(115200);
    delay(2000); // Longer delay for serial monitor to stabilize

    Serial.println("=====================================");
    Serial.println("ESP32X Robot Controller Starting...");
    Serial.flush(); // Force output
    delay(100);

    Serial.println("=====================================");
    Serial.flush();
    delay(100);

    Serial.println("=== COLOR SENSOR TEST MODE ===");
    Serial.flush();
    delay(100);

    Serial.println("Place different colors in front of sensor to test detection");
    Serial.flush();
    delay(100);

    Serial.println("RED color will move servo and print messages");
    Serial.flush();
    delay(100);

    Serial.println("=====================================");
    Serial.flush();
    delay(100);

    Serial.println("Initializing motor pins...");
    Serial.flush();
    // Initialize motor pins
    pinMode(MOTOR_LEFT_PWM, OUTPUT);
    pinMode(MOTOR_LEFT_DIR, OUTPUT);
    pinMode(MOTOR_RIGHT_PWM, OUTPUT);
    pinMode(MOTOR_RIGHT_DIR, OUTPUT);
    Serial.println("Motor pins initialized OK");
    Serial.flush();
    delay(100);

    Serial.println("Initializing IR sensor pins...");
    Serial.flush();
    // Initialize IR sensor pins
    pinMode(IR_LEFT, INPUT);
    pinMode(IR_RIGHT, INPUT);
    Serial.println("IR sensor pins initialized OK");
    Serial.flush();
    delay(100);

    Serial.println("Initializing ultrasonic sensor pins...");
    Serial.flush();
    // Initialize ultrasonic sensor pins
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    Serial.println("Ultrasonic sensor pins initialized OK");
    Serial.flush();
    delay(100);

    Serial.println("Initializing color sensor pins...");
    Serial.flush();
    // Initialize color sensor pins
    pinMode(COLOR_S0, OUTPUT);
    pinMode(COLOR_S1, OUTPUT);
    pinMode(COLOR_S2, OUTPUT);
    pinMode(COLOR_S3, OUTPUT);
    pinMode(COLOR_OUT, INPUT);

    // Set color sensor frequency scaling to 20%
    digitalWrite(COLOR_S0, HIGH);
    digitalWrite(COLOR_S1, LOW);
    Serial.println("Color sensor pins configured OK");
    Serial.flush();
    delay(100);

    Serial.println("Initializing servo...");
    Serial.flush();

    // Try-catch equivalent for servo initialization
    bool servoOK = false;
    for (int attempts = 0; attempts < 3; attempts++)
    {
        Serial.printf("Servo init attempt %d...\n", attempts + 1);
        Serial.flush();

        if (ledcAttach(SERVO_PIN, servoFreq, 16))
        {
            servoOK = true;
            Serial.println("Servo channel attached successfully");
            Serial.flush();
            break;
        }
        else
        {
            Serial.printf("Servo attach failed, attempt %d\n", attempts + 1);
            Serial.flush();
            delay(500);
        }
    }

    if (servoOK)
    {
        Serial.println("Moving servo to center position...");
        Serial.flush();
        servoWrite(90); // Center position
        delay(1000);    // Allow servo to reach position
        Serial.println("Servo initialized successfully at 90 degrees");
        Serial.flush();
    }
    else
    {
        Serial.println("WARNING: Servo initialization failed - continuing without servo");
        Serial.flush();
    }

    Serial.println("Stopping motors...");
    Serial.flush();
    // Stop motors initially
    stopMotors();
    Serial.println("Motors stopped OK");
    Serial.flush();

    Serial.println("=====================================");
    Serial.println("ESP32X Robot Controller READY!");
    Serial.println("Starting COLOR DETECTION TEST...");
    Serial.println("=====================================");
    Serial.flush();

    // Calibrate color sensor
    calibrateColorSensor();

    Serial.println("Setup complete - entering main loop");
    Serial.flush();
}

void loop()
{
    // Watchdog - print heartbeat every 5 seconds
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat > 5000)
    {
        Serial.printf("Heartbeat: %lu ms - System running OK\n", millis());
        Serial.flush();
        lastHeartbeat = millis();
    }

    // Get current traffic light status (with timing control)
    TrafficLightColor currentLight = NONE;
    if (millis() - lastColorCheck >= colorCheckInterval)
    {
        currentLight = detectTrafficLight();
        lastColorCheck = millis();

        // Enhanced RED detection tracking and messaging
        if (currentLight == RED)
        {
            if (!lastRedState) // Red just detected (transition from no red to red)
            {
                redDetectionCount++;
                Serial.println("");
                Serial.println("🔴🔴🔴 RED COLOR DETECTED! 🔴🔴🔴");
                Serial.printf("Detection #%lu at time: %lu ms\n", redDetectionCount, millis());
                Serial.println("🔴🔴🔴 TRIGGERING SERVO TEST 🔴🔴🔴");
                Serial.println("");
                Serial.flush();
                lastRedState = true;
            }
            testServoMovement();
        }
        else
        {
            if (lastRedState) // Red just lost
            {
                Serial.println("🟢 Red color no longer detected");
                Serial.flush();
                lastRedState = false;
            }
        }
    }

    // State machine
    switch (currentState)
    {
    case COLOR_TEST_MODE:
    {
        // Enhanced test mode output
        if (currentLight != NONE)
        {
            Serial.printf("⚡ TEST MODE - Color Detected: ");
            switch (currentLight)
            {
            case RED:
                Serial.println("🔴 RED");
                break;
            case GREEN:
                Serial.println("🟢 GREEN");
                break;
            case BLUE:
                Serial.println("🔵 BLUE");
                break;
            default:
                Serial.println("❓ UNKNOWN");
                break;
            }
            Serial.flush();
        }

        // Uncomment the next line to switch to normal robot mode after testing
        // if (millis() > 30000) currentState = LINE_FOLLOWING; // Switch after 30 seconds
    }
    break;

    case LINE_FOLLOWING:
    {
        // Check for obstacles
        int distance = measureDistance();

        Serial.printf("Distance: %dcm, Light: %d, State: %d\n", distance, currentLight, currentState);

        if (distance < OBSTACLE_THRESHOLD && distance > 0)
        {
            currentState = OBSTACLE_AVOIDANCE;
            Serial.println("OBSTACLE DETECTED - Switching to avoidance mode");
            stopMotors();
            delay(500);
        }
        else if (currentLight == RED)
        {
            currentState = TRAFFIC_LIGHT_STOP;
            Serial.println("🔴 RED LIGHT DETECTED - STOPPING ROBOT");
            stopMotors();
        }
        else if (currentLight == BLUE) // Changed from YELLOW to BLUE
        {
            currentState = TRAFFIC_LIGHT_SLOW;
            Serial.println("🔵 BLUE LIGHT DETECTED - Slowing down");
        }
        else
        {
            followLine();
        }
    }
    break;

    case OBSTACLE_AVOIDANCE:
    {
        int distance2 = measureDistance();
        if (distance2 >= OBSTACLE_THRESHOLD || distance2 == 0)
        {
            currentState = LINE_FOLLOWING;
            Serial.println("OBSTACLE CLEARED - Resuming line following");
            servoWrite(90); // Reset servo to center
            delay(500);
        }
        else
        {
            avoidObstacle();
        }
    }
    break;

    case TRAFFIC_LIGHT_STOP:
    {
        stopMotors();
        if (currentLight == GREEN)
        {
            currentState = LINE_FOLLOWING;
            Serial.println("🟢 GREEN LIGHT DETECTED - Resuming movement");
        }
        else if (currentLight == NONE)
        {
            // If no light detected for a while, resume movement
            static unsigned long noLightTime = 0;
            if (noLightTime == 0)
                noLightTime = millis();
            if (millis() - noLightTime > 3000) // 3 seconds
            {
                currentState = LINE_FOLLOWING;
                Serial.println("NO LIGHT TIMEOUT - Resuming movement");
                noLightTime = 0;
            }
        }
        else
        {
            static unsigned long noLightTime = 0;
            noLightTime = 0; // Reset timer if light is detected
        }
    }
    break;

    case TRAFFIC_LIGHT_SLOW:
    {
        if (currentLight == GREEN)
        {
            currentState = LINE_FOLLOWING;
            Serial.println("🟢 GREEN LIGHT DETECTED - Resuming normal speed");
        }
        else if (currentLight == RED)
        {
            currentState = TRAFFIC_LIGHT_STOP;
            Serial.println("🔴 RED LIGHT DETECTED - STOPPING");
            stopMotors();
        }
        else
        {
            followLineSlow();
        }
    }
    break;
    }

    delay(50); // Main loop delay
}

void testServoMovement()
{
    Serial.println("");
    Serial.println("🤖 *** SERVO MOVEMENT TEST INITIATED ***");
    Serial.println("🔴 RED DETECTED - EXECUTING SERVO SEQUENCE");
    Serial.flush();

    // Move servo to 0 degrees
    Serial.println("📍 Step 1: Moving servo to 0 degrees...");
    Serial.flush();
    servoWrite(0);
    delay(1000);

    // Move servo to 180 degrees
    Serial.println("📍 Step 2: Moving servo to 180 degrees...");
    Serial.flush();
    servoWrite(180);
    delay(1000);

    // Return to center
    Serial.println("📍 Step 3: Returning servo to center (90 degrees)...");
    Serial.flush();
    servoWrite(90);
    delay(1000);

    Serial.println("✅ SERVO MOVEMENT TEST COMPLETE!");
    Serial.println("🔴 Red detection servo test finished successfully");
    Serial.println("");
    Serial.flush();
}

void calibrateColorSensor()
{
    Serial.println("📊 Color sensor calibration info:");
    Serial.printf("🎯 RED threshold: < %lu\n", RED_DETECT_THRESHOLD);
    Serial.printf("🎯 BLUE threshold: < %lu\n", BLUE_DETECT_THRESHOLD);
    Serial.printf("🎯 GREEN threshold: < %lu\n", GREEN_DETECT_THRESHOLD);
    Serial.println("⚡ Sensor ready for color detection!");
    Serial.println("=====================================");
    Serial.flush();
}

TrafficLightColor detectTrafficLight()
{
    // Read RGB frequencies with error handling
    redFrequency = getColorFrequency('R');
    if (redFrequency == 0)
        redFrequency = 999; // Handle timeout

    greenFrequency = getColorFrequency('G');
    if (greenFrequency == 0)
        greenFrequency = 999; // Handle timeout

    blueFrequency = getColorFrequency('B');
    if (blueFrequency == 0)
        blueFrequency = 999; // Handle timeout

    // Print color values occasionally for debugging
    static unsigned long lastDebugPrint = 0;
    if (millis() - lastDebugPrint > 2000) // Every 2 seconds
    {
        Serial.printf("📊 RGB: R=%lu G=%lu B=%lu\n", redFrequency, greenFrequency, blueFrequency);
        Serial.flush();
        lastDebugPrint = millis();
    }

    // Check for RED (red frequency should be lowest and < threshold)
    if (redFrequency < RED_DETECT_THRESHOLD &&
        redFrequency < greenFrequency - COLOR_DIFFERENCE_MIN &&
        redFrequency < blueFrequency - COLOR_DIFFERENCE_MIN)
    {
        return RED;
    }

    // Check for GREEN (green frequency should be lowest and < threshold)
    if (greenFrequency < GREEN_DETECT_THRESHOLD &&
        greenFrequency < redFrequency - COLOR_DIFFERENCE_MIN &&
        greenFrequency < blueFrequency - COLOR_DIFFERENCE_MIN)
    {
        return GREEN;
    }

    // Check for BLUE (blue frequency should be lowest and < threshold)
    if (blueFrequency < BLUE_DETECT_THRESHOLD &&
        blueFrequency < redFrequency - COLOR_DIFFERENCE_MIN &&
        blueFrequency < greenFrequency - COLOR_DIFFERENCE_MIN)
    {
        return BLUE;
    }

    return NONE;
}

unsigned long getColorFrequency(char color)
{
    // Set color filter
    switch (color)
    {
    case 'R': // Red
        digitalWrite(COLOR_S2, LOW);
        digitalWrite(COLOR_S3, LOW);
        break;
    case 'G': // Green
        digitalWrite(COLOR_S2, HIGH);
        digitalWrite(COLOR_S3, HIGH);
        break;
    case 'B': // Blue
        digitalWrite(COLOR_S2, LOW);
        digitalWrite(COLOR_S3, HIGH);
        break;
    default:
        return 0;
    }

    // Small delay for filter switching
    delayMicroseconds(100);

    // Read frequency with timeout
    unsigned long frequency = pulseIn(COLOR_OUT, LOW, 30000); // 30ms timeout (reduced)
    return frequency;
}

void servoWrite(int angle)
{
    // Constrain angle to valid range
    angle = constrain(angle, 0, 180);
    currentServoAngle = angle;

    // Convert angle (0-180) to duty cycle for 50Hz PWM (1ms-2ms pulse width)
    int pulseWidth = map(angle, 0, 180, 1000, 2000); // 1000-2000 microseconds
    int dutyCycle = (pulseWidth * 65536) / 20000;    // 20000us = 20ms period for 50Hz
    ledcWrite(SERVO_PIN, dutyCycle);

    Serial.printf("🔧 Servo moved to %d degrees\n", angle);
    Serial.flush();
}

void followLine()
{
    bool leftSensor = digitalRead(IR_LEFT);
    bool rightSensor = digitalRead(IR_RIGHT);

    // IR sensors: LOW = black line detected, HIGH = white surface
    if (!leftSensor && !rightSensor)
    {
        // Both sensors on black line - go straight
        moveForward(NORMAL_SPEED);
        Serial.println("Following line: STRAIGHT");
    }
    else if (!leftSensor && rightSensor)
    {
        // Left sensor on line, right sensor off - turn left
        turnLeft(TURN_SPEED);
        Serial.println("Following line: TURN LEFT");
    }
    else if (leftSensor && !rightSensor)
    {
        // Right sensor on line, left sensor off - turn right
        turnRight(TURN_SPEED);
        Serial.println("Following line: TURN RIGHT");
    }
    else
    {
        // Both sensors off line - stop and search
        stopMotors();
        Serial.println("Following line: LINE LOST - SEARCHING");

        // Search pattern
        static bool searchLeft = true;
        if (searchLeft)
        {
            turnLeft(TURN_SPEED);
            delay(200);
        }
        else
        {
            turnRight(TURN_SPEED);
            delay(200);
        }
        searchLeft = !searchLeft;
    }
}

void followLineSlow()
{
    bool leftSensor = digitalRead(IR_LEFT);
    bool rightSensor = digitalRead(IR_RIGHT);

    if (!leftSensor && !rightSensor)
    {
        moveForward(SLOW_SPEED);
        Serial.println("Following line SLOW: STRAIGHT");
    }
    else if (!leftSensor && rightSensor)
    {
        turnLeft(SLOW_SPEED);
        Serial.println("Following line SLOW: TURN LEFT");
    }
    else if (leftSensor && !rightSensor)
    {
        turnRight(SLOW_SPEED);
        Serial.println("Following line SLOW: TURN RIGHT");
    }
    else
    {
        stopMotors();
        Serial.println("Following line SLOW: LINE LOST");
        delay(200);
    }
}

void avoidObstacle()
{
    Serial.println("AVOIDING OBSTACLE...");
    stopMotors();
    delay(500);

    // Turn servo to scan left
    Serial.println("Scanning LEFT...");
    servoWrite(135); // Look left
    delay(800);
    int leftDistance = measureDistance();
    Serial.printf("Left distance: %dcm\n", leftDistance);

    // Turn servo to scan right
    Serial.println("Scanning RIGHT...");
    servoWrite(45); // Look right
    delay(800);
    int rightDistance = measureDistance();
    Serial.printf("Right distance: %dcm\n", rightDistance);

    // Return servo to center
    servoWrite(90);
    delay(500);

    // Choose direction based on clearance
    if (leftDistance > rightDistance && leftDistance > OBSTACLE_THRESHOLD)
    {
        Serial.println("Avoiding LEFT...");
        // Turn left to avoid
        turnLeft(TURN_SPEED);
        delay(800);
        moveForward(NORMAL_SPEED);
        delay(1200);
        turnRight(TURN_SPEED);
        delay(800);
        moveForward(NORMAL_SPEED);
        delay(800);
        turnRight(TURN_SPEED);
        delay(800);
    }
    else if (rightDistance > leftDistance && rightDistance > OBSTACLE_THRESHOLD)
    {
        Serial.println("Avoiding RIGHT...");
        // Turn right to avoid
        turnRight(TURN_SPEED);
        delay(800);
        moveForward(NORMAL_SPEED);
        delay(1200);
        turnLeft(TURN_SPEED);
        delay(800);
        moveForward(NORMAL_SPEED);
        delay(800);
        turnLeft(TURN_SPEED);
        delay(800);
    }
    else
    {
        Serial.println("No clear path - BACKING UP...");
        // Backup and try again
        moveBackward(NORMAL_SPEED);
        delay(1000);
        turnLeft(TURN_SPEED);
        delay(500);
    }
}

int measureDistance()
{
    // Send ultrasonic pulse
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Measure echo duration with timeout
    long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout

    if (duration == 0)
    {
        return 999; // Return large value if no echo received
    }

    // Calculate distance in cm
    int distance = duration * 0.034 / 2;

    // Filter out invalid readings
    if (distance < 2 || distance > 400)
    {
        return 999;
    }

    return distance;
}

void moveForward(int speed)
{
    speed = constrain(speed, 0, 255);
    digitalWrite(MOTOR_LEFT_DIR, HIGH);
    digitalWrite(MOTOR_RIGHT_DIR, HIGH);
    analogWrite(MOTOR_LEFT_PWM, speed);
    analogWrite(MOTOR_RIGHT_PWM, speed);
}

void moveBackward(int speed)
{
    speed = constrain(speed, 0, 255);
    digitalWrite(MOTOR_LEFT_DIR, LOW);
    digitalWrite(MOTOR_RIGHT_DIR, LOW);
    analogWrite(MOTOR_LEFT_PWM, speed);
    analogWrite(MOTOR_RIGHT_PWM, speed);
}

void turnLeft(int speed)
{
    speed = constrain(speed, 0, 255);
    digitalWrite(MOTOR_LEFT_DIR, LOW);   // Left motor backward
    digitalWrite(MOTOR_RIGHT_DIR, HIGH); // Right motor forward
    analogWrite(MOTOR_LEFT_PWM, speed);
    analogWrite(MOTOR_RIGHT_PWM, speed);
}

void turnRight(int speed)
{
    speed = constrain(speed, 0, 255);
    digitalWrite(MOTOR_LEFT_DIR, HIGH); // Left motor forward
    digitalWrite(MOTOR_RIGHT_DIR, LOW); // Right motor backward
    analogWrite(MOTOR_LEFT_PWM, speed);
    analogWrite(MOTOR_RIGHT_PWM, speed);
}

void stopMotors()
{
    analogWrite(MOTOR_LEFT_PWM, 0);
    analogWrite(MOTOR_RIGHT_PWM, 0);
    digitalWrite(MOTOR_LEFT_DIR, LOW);
    digitalWrite(MOTOR_RIGHT_DIR, LOW);
}