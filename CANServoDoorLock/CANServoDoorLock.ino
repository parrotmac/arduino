#include <Servo.h>
#include <SPI.h>
#include <mcp2515.h>

const int LOCK_FRAME_CAN_ID = 0x77;
const int LOCK_FRAME_CAN_LENGTH = 0x01;
const int LOCK_FRAME_ACTION_LOCK = 0x05;
const int LOCK_FRAME_ACTION_UNLOCK = 0x0A;

struct can_frame canMsg;
MCP2515 mcp2515(10);

Servo wristServo;
const int wristServoPin = 8;
int wristPos = 180;

Servo thumbServo;
const int thumbServoPin = 7;
const int thumbInitalPos = 160;

const int READ_BUFFER_SIZE = 20;
char readBuf[READ_BUFFER_SIZE] = {};
int readBufPos = 0;
const int CMD_POS_OFFSET = 2; // First char is servo, second separator, everything after is position
const int CMD_POS_BUF_LEN = 4;
char posBuf[CMD_POS_BUF_LEN]; // Place to store incoming position. Last char is null-terminator

void setup() {
  Serial.begin(115200);

  Serial.print("Initializing CAN Bus...");
  SPI.begin();
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
  Serial.println("Done");

  
  wristServo.attach(wristServoPin);
  thumbServo.attach(thumbServoPin);
  
  thumbServo.write(thumbInitalPos);

  Serial.print("Initializing Wrist servo...");
  for(int i = 30; i < 180; i++) {
    wristServo.write(i);
    delay(15);
  }
  
  for(int i = 180; i <= 30; i--) {
    wristServo.write(i);
    delay(15);
  }

  
  for(int i = 30; i < 180; i++) {
    wristServo.write(i);
    delay(15);
  }
  Serial.println("Done");
}

void resetBuffer() {
  readBufPos = 0;
  memset(readBuf, '\0', READ_BUFFER_SIZE);
}

void writeWristPos(int pos) {
  delay(15);
  wristServo.write(pos);
}

void setWristPosition(int desiredPos) {
    Serial.print(">W:");
    if (desiredPos > wristPos) {
      // increment actual position until it reaches target
      for(int i = wristPos + 1; i <= desiredPos; i++) {
        writeWristPos(i);
      }
    }
    if (desiredPos < wristPos) {
      // decrement actuaio position
      for(int i = wristPos - 1; i >= desiredPos; i--) {
        writeWristPos(i);
      }
    }
    wristPos = desiredPos;
    Serial.print(wristPos);
    Serial.print('\r');
}

void unlock() {
          thumbServo.write(50);
          setWristPosition(12);
          delay(1000);
          thumbServo.write(160);
          delay(500);
          setWristPosition(180);
}

void lock() {
          thumbServo.write(160);
          setWristPosition(12);
          delay(1000);
          thumbServo.write(50);
          delay(500);
          setWristPosition(180);
          thumbServo.write(160);
}

void loop() {

  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.print(canMsg.can_id, HEX); // print ID
    Serial.print("["); Serial.print(LOCK_FRAME_CAN_ID, HEX); Serial.print("]"); // DEBUG -- print expected value
    Serial.print(" ");
    Serial.print(canMsg.can_dlc, HEX); // print DLC
    Serial.print("["); Serial.print(LOCK_FRAME_CAN_LENGTH, HEX); Serial.print("]"); // DEBUG -- print expected value
    Serial.print(" ");
    for (int i = 0; i<canMsg.can_dlc; i++)  {  // print the data
      Serial.print(canMsg.data[i],HEX);
      Serial.print("[U?"); Serial.print(canMsg.data[i] == LOCK_FRAME_ACTION_UNLOCK); Serial.print("]"); // DEBUG -- equals unlock?
      Serial.print("[L?"); Serial.print(canMsg.data[i] == LOCK_FRAME_ACTION_LOCK); Serial.print("]"); // DEBUG -- equals lock?
      Serial.print(" ");
    }

    if(canMsg.can_id == LOCK_FRAME_CAN_ID && canMsg.can_dlc == LOCK_FRAME_CAN_LENGTH) {
      switch(canMsg.data[0]) {
        case LOCK_FRAME_ACTION_UNLOCK:
          Serial.println("unlockin'");
          unlock();
          break;
        case LOCK_FRAME_ACTION_LOCK:
          Serial.println("lockin'");
          lock();
          break;
        default:
          Serial.println("Lol idk what that means");
      }
    } else {
      Serial.println("Not a thing");
    }
    
  }
  
  if(Serial.available() > 0) {
    char readByte = Serial.read();
    if (readByte == '\r') {
      // \r -- process
      const char dstServo = readBuf[0];

      memset(posBuf, '\0', CMD_POS_BUF_LEN); // Reset posBuf before use
      
      memcpy(&posBuf[0], &readBuf[CMD_POS_OFFSET], CMD_POS_BUF_LEN - 1);
      
      const int reqPos = atoi(posBuf);

      switch(dstServo) {
        case 'T':
          Serial.print(">T:");
          Serial.print(reqPos);
          Serial.print('\r');
          thumbServo.write(reqPos);
        break;
        case 'W':
          setWristPosition(reqPos);
        break;
        case 'U':
          unlock();
          break;
        case 'L':
          lock();
          break;
        default:
          Serial.println("That's not a recognized command");
          break;
      }
      
      resetBuffer();
    } else {
      if (readBufPos < READ_BUFFER_SIZE - 1) {
        readBuf[readBufPos++] = readByte;
      } else {
        Serial.println("ERR: Exceeded incoming buffer length");
      }
    }
  }
}
