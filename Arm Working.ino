#include <ESP32Servo.h>




// ===== Servo Objects =====
Servo baseServo;
Servo shoulderServo;
Servo elbowServo;
Servo gripperServo;




// ===== IR Sensor Pins =====
#define FOOD_IR_PIN 26   // Food available sensor
#define PLATE_IR_PIN 25  // Plate available sensor




// ===== Servo Positions =====
int basePos = 90;
int shoulderPos = 90;
int elbowPos = 180;
int gripperPos = 90;




int moveDelay = 15;
bool objectPicked = false;




// ===== Smooth Servo Movement Function =====
void moveServo(Servo &servo, int &currentPos, int targetPos) {
  if (currentPos < targetPos) {
    for (int i = currentPos; i <= targetPos; i++) {
      servo.write(i);
      delay(moveDelay);
    }
  } else {
    for (int i = currentPos; i >= targetPos; i--) {
      servo.write(i);
      delay(moveDelay);
    }
  }
  currentPos = targetPos;
}




// ===== Home Position =====
void homePosition() {
  moveServo(baseServo, basePos, 90);
  moveServo(shoulderServo, shoulderPos, 90);
  moveServo(elbowServo, elbowPos, 125);
  moveServo(gripperServo, gripperPos, 90);
}




// ===== Setup =====
void setup() {
  Serial.begin(115200);




  // Attach Servos
  baseServo.attach(13);
  shoulderServo.attach(12);
  elbowServo.attach(14);
  gripperServo.attach(27);




  // IR Sensors
  pinMode(FOOD_IR_PIN, INPUT);
  pinMode(PLATE_IR_PIN, INPUT);




  Serial.println("🤖 Robot Arm Starting...");
  homePosition();
  delay(2000);
  Serial.println("✅ System Ready (Waiting for food & plate)");
}




// ===== Main Loop =====
void loop() {
  int food = digitalRead(FOOD_IR_PIN);   // LOW = detected
  int plate = digitalRead(PLATE_IR_PIN); // LOW = detected




  // Debug status
  Serial.print("Food: ");
  Serial.print(food == LOW ? "YES" : "NO");
  Serial.print(" | Plate: ");
  Serial.println(plate == LOW ? "YES" : "NO");




  // If both food & plate detected
  if (food == LOW && plate == LOW && !objectPicked) {
    Serial.println("🍔 Food + 🍽️ Plate detected! Serving...");




    // Move to food position
    moveServo(baseServo, basePos, 90);
    moveServo(shoulderServo, shoulderPos, 120);
    moveServo(elbowServo, elbowPos, 135);




    // Grab food
    moveServo(gripperServo, gripperPos, 0);
    delay(500);




    // Lift food
    moveServo(shoulderServo, shoulderPos, 60);
    moveServo(elbowServo, elbowPos, 180);




    // Move to plate position
    moveServo(baseServo, basePos, 180);




    // Place food on plate
    moveServo(gripperServo, gripperPos, 110);
    delay(500);




    // Return home
    homePosition();




    Serial.println("✅ Food served successfully!");
    objectPicked = true;
  }




  // Reset when food or plate removed
  if (food == HIGH || plate == HIGH) {
    objectPicked = false;
  }




  delay(200); // small delay for stability
}



