#include <SPI.h>
#include <MFRC522.h>
#include <BluetoothSerial.h>


BluetoothSerial SerialBT;


// ================= MOTOR PINS =================
#define ENA 25
#define IN1 26
#define IN2 27
#define ENB 14
#define IN3 16
#define IN4 17


// ================= LINE IR SENSORS =================
#define IR1 36
#define IR2 39
#define IR3 34
#define IR4 35
#define IR5 33


// ================= FOOD SENSOR =================
#define IR_OBJECT 2   // LOW = food present


// ================= ULTRASONIC =================
#define TRIG 32
#define ECHO 4
#define OBSTACLE_DISTANCE 15


// ================= PWM =================
#define PWM_FREQ 1000
#define PWM_RES 8
#define PWM_CH_A 0
#define PWM_CH_B 1


// ================= SPEED =================
int baseSpeed     = 80;
int turnSpeed     = 120;
int hardTurnSpeed = 150;
int rcSpeed       = 120;


// ================= RFID =================
#define RFID_SDA 21
#define RFID_RST 22
MFRC522 rfid(RFID_SDA, RFID_RST);


#define UID_HOME "23 35 97 31"
#define UID_2 "75 54 AA AC"
#define UID_3 "A2 75 D1 1B"
#define UID_4 "53 87 F0 31"


String activeTargetUID = "";
bool returningHome = false;


unsigned long lastRFIDTime = 0;
const unsigned long RFID_COOLDOWN = 2000;


// ================= ROBOT MODE =================
enum RobotMode {
  MODE_IDLE,
  MODE_AUTO,
  MODE_RC
};


RobotMode currentMode = MODE_IDLE;


// ================= ULTRASONIC =================
unsigned long lastUltrasonicTime = 0;
const unsigned long ULTRASONIC_INTERVAL = 60;
int lastDistance = -1;
bool obstacleActive = false;


// ================= LINE MEMORY ================= -1, 0 , 1
int lastTurn = 0; 


// ================= BUZZER =================
#define BUZZER_PIN 13


// ===================================================
void setup() {


  Serial.begin(115200);
  SerialBT.begin("LineFollowerESP32");


  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);


  pinMode(IR1, INPUT);
  pinMode(IR2, INPUT);
  pinMode(IR3, INPUT);
  pinMode(IR4, INPUT);
  pinMode(IR5, INPUT);


  pinMode(IR_OBJECT, INPUT_PULLUP);


  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);


  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);


  ledcSetup(PWM_CH_A, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CH_B, PWM_FREQ, PWM_RES);
  ledcAttachPin(ENA, PWM_CH_A);
  ledcAttachPin(ENB, PWM_CH_B);


  SPI.begin(18, 19, 23, RFID_SDA);
  rfid.PCD_Init();


  stopMotors();


  SerialBT.println("Commands: 2 3 4 STOP RETURN RC");
}


// ===================================================
void loop() {


  bool foodDetected = (digitalRead(IR_OBJECT) == LOW);


  // ================= BLUETOOTH =================
  if (SerialBT.available()) {


    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();


    if (cmd == "2") {
      currentMode = MODE_AUTO;
      activeTargetUID = UID_2;
      returningHome = false;
      stopMotors();
      beep(100);
    }


    else if (cmd == "3") {
      currentMode = MODE_AUTO;
      activeTargetUID = UID_3;
      returningHome = false;
      stopMotors();
      beep(100);
    }


    else if (cmd == "4") {
      currentMode = MODE_AUTO;
      activeTargetUID = UID_4;
      returningHome = false;
      stopMotors();
      beep(100);
    }


    else if (cmd == "STOP") {
      currentMode = MODE_IDLE;
      stopMotors();
      beep(200);
    }


    else if (cmd == "RETURN") {
      stopMotors();
      beep(150);
      turnAround180();
      returningHome = true;
    }


    else if (cmd == "RC") {
      currentMode = MODE_RC;
      stopMotors();
      beep(120);
    }


    else if (currentMode == MODE_RC) {
      if (cmd == "F") forward(rcSpeed, rcSpeed);
      else if (cmd == "B") backward(rcSpeed);
      else if (cmd == "L") leftTurn(rcSpeed);
      else if (cmd == "R") rightTurn(rcSpeed);
      else if (cmd == "S") stopMotors();
    }
  }


  // ================= MODE CHECK =================
  if (currentMode == MODE_IDLE) {
    stopMotors();
    return;
  }


  if (currentMode == MODE_RC) {
    return;
  }


  // ===== FOOD required only when going to delivery =====
  if (!returningHome && !foodDetected) {
    stopMotors();
    return;
  }


  // ================= ULTRASONIC =================
  if (millis() - lastUltrasonicTime >= ULTRASONIC_INTERVAL) {
    lastUltrasonicTime = millis();
    lastDistance = readDistanceCM();
  }


  if (lastDistance > 0 && lastDistance <= OBSTACLE_DISTANCE) {
    stopMotors();
    if (!obstacleActive) {
      beep(80);
      obstacleActive = true;
    }
    return;
  } else {
    obstacleActive = false;
  }


  // ================= RFID =================
  if (millis() - lastRFIDTime > RFID_COOLDOWN) {


    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {


      String uidStr = "";
      for (byte i = 0; i < rfid.uid.size; i++) {
        if (rfid.uid.uidByte[i] < 0x10) uidStr += "0";
        uidStr += String(rfid.uid.uidByte[i], HEX);
        uidStr += " ";
      }


      uidStr.toUpperCase();
      uidStr.trim();


      SerialBT.print("RFID: ");
      SerialBT.println(uidStr);


      // ===== DELIVERY POINT =====
      if (!returningHome && uidStr == activeTargetUID) {


        stopMotors();
        beep(200);


        SerialBT.println("Arrived. Waiting for food pickup...");


        while (digitalRead(IR_OBJECT) == LOW) {
          delay(50);
        }


        SerialBT.println("Food picked up. Returning home.");
        beep(300);
        delay(500);


        turnAround180();
        returningHome = true;
      }


      // ===== HOME POINT =====
      else if (returningHome && uidStr == UID_HOME) {


        stopMotors();
        beep(400);


        SerialBT.println("Arrived Home. Resetting orientation...");
        delay(500);


        turnAround180();   // Face delivery direction again


        currentMode = MODE_IDLE;
        returningHome = false;


        SerialBT.println("Ready for next pickup.");
      }


      lastRFIDTime = millis();
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }

