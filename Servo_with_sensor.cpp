// Included the library to work with servo motors
#include <Servo.h>

// Included the pins for the ultrasonic sensor and servo
const int TRIG_PIN = 12;
const int ECHO_PIN = 11;
const int SERVO_PIN = 9;

// Configs to servo and sensor
const int MAX_DISTANCE = 200; // Maximum distance for the sensor to detect
const int MIN_DISTANCE = 2; // Minimum distance for the sensor to detect
const int SERVO_MIN_ANGLE = 0; // Minimum angle for the servo
const int SERVO_MAX_ANGLE = 180; // Maximum angle for the servo

// Filter to stabilize the readings
const int NUM_READINGS = 5; // Number of readings to average
long readings[NUM_READINGS]; // Array to store the readings
int readIndex = 0; // Current index in the readings array
long total = 0; // Total of the readings

Servo myServo; // Create a Servo object

void setup(){

    Serial.begin(9600); // Start serial communication at 9600 baud rate
    myServo.attach(SERVO_PIN); // Attach the servo to the specified pin
    pinMode(TRIG_PIN, OUTPUT); // Set the trigger pin as output
    pinMode(ECHO_PIN, INPUT); // Set the echo pin as input

    // Guarantee the trigger pin is low
    digitalWrite(TRIG_PIN, LOW);

    // Initialize the readings array
    for (int i = 0; i < NUM_READINGS; i++) {
        readings[i] = 0;
    }
    
    myServo.write(90); // Initialize servo to minimum angle
    delay(1000); // Wait for the servo to reach the position

    Serial.println("Servo and sensor initialized.");
    Serial.println("Waiting the stabilization...");
    delay(2000); // Wait for the sensor to stabilize
}

void loop(){
    // Measure the distance with filtering
    long distance = measureDistanceWithFilter();

    // Detailed debug
    Serial.print("Distância bruta: ");
    Serial.print(measureDistanceRaw());
    Serial.print(" cm");
    Serial.println("Distância filtrada: ");
    Serial.print(distance);
    Serial.print(" cm");

    // Verify if the distance is valid
    if (distance > 0 && distance <= MAX_DISTANCE) {
        // Mapping the distance to the servo angle
        int servoAngle = mapDistanceToAngle(distance);

        // Move the servo to the calculated angle
        myServo.write(servoAngle);

        Serial.print("Ângulo: ");
        Serial.print(servoAngle);
        Serial.print("°");
    } 
    else {
        Serial.print(" | Leitura inválida - mantendo posição");
    }

    delay(200); // Delay to bigger stabilization

}

long measureDistanceRaw(){
    // Clean the trigger pin
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(5);

    // Send the trigger pulse
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(15);
    digitalWrite(TRIG_PIN, LOW);

    // Read the echo pulse with 35ms timeout
    long duration = pulseIn(ECHO_PIN, HIGH, 35000); 

    // If no pulse is received, return 0
    if (duration == 0) {
       Serial.println("TIMEOUT");
       return -1; // Indicate error
    }

    // Calculate the distance in cm
    long distance = (duration * 343) / (2 * 10000);

    return distance;
}

long measureDistanceWithFilter(){
    // Subtract the last reading
    total -= readings[readIndex];

    // Read the distance
    long distance = measureDistanceRaw();
    
    // If there was an error, return the last valid average
    if (distance <= 0 || distance > MAX_DISTANCE) {
        distance = readings[readIndex];
    }

    // Store the reading in the array
    readings[readIndex] = distance;

    // Add the reading to the total
    total += readings[readIndex];

    // Advance to the next index
    readIndex = (readIndex + 1) % NUM_READINGS;

    // Return the average
    return total / NUM_READINGS;
}

int mapDistanceToAngle(long distance){
    // Guarantee the distance is within the sensor limits
    distance = constrain(distance, MIN_DISTANCE, MAX_DISTANCE);

    // Mapping the distance to the servo angle
    long servoAngle = map(distance, MIN_DISTANCE, MAX_DISTANCE, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);

    // Mapping the debug
    Serial.print("[Map: ");
    Serial.print(distance);
    Serial.print(" cm");
    Serial.println(servoAngle);
    Serial.println("°]");

    // Guarantee the angle is within the servo limits
    return constrain(servoAngle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
}