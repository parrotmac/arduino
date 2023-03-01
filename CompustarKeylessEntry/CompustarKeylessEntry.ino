#include <EEPROM.h>
#include <SoftwareSerial.h>

const char CMD_LOCK = 0x30;
const char CMD_UNLOCK = 0x31;
const char CMD_HOLD_TRUNK = 0x34;
const char CMD_HOLD_REMOTE_START = 0x32;

const int LED_PIN = 13;

const int LOCK_PIN = 5;
const int UNLOCK_PIN = 6;
const int HORN_PIN = 7;
const int PARKING_LIGHTS_PIN = 8;

const int PRGM_BTN_INTERRUPT_PIN = 2;

const int DOORLOCK_PULSE_MS = 500;
const int HORN_PULSE_MS = 500;
const int PARKING_LIGHTS_PULSE_MS = 500;

const int BUFFER_SIZE = 30;
int bufferPos = 0;
char incomingBuffer[BUFFER_SIZE];
char readData;

bool firstLockHit = false;

bool prgmBtnDn = false;
long prgmBtnDnAt;

#define MAX_REMOTES 4
int currRemoteIdx = 0;
uint8_t incomingRemoteId[3];
uint8_t trustedRemoteIds[MAX_REMOTES][3];
int remoteEEPROMOffset = 5;

void prgmBtnISR() {
  prgmBtnDn = digitalRead(PRGM_BTN_INTERRUPT_PIN) == LOW;
  if (prgmBtnDn) {
    prgmBtnDnAt = millis();
  }
}

void saveRemoteAddressIfNeeded() {

  if (currRemoteIdx + 1 >= MAX_REMOTES) {
    currRemoteIdx = 0;
  }

  if (prgmBtnDn && prgmBtnDnAt + 250 <= millis()) {
    Serial.print("[DEBUG] Saving remote ID: ");
    for (int i = 0; i < 3; i++) {
      trustedRemoteIds[currRemoteIdx][i] = (uint8_t)incomingBuffer[5 + i];
    }
    for (int i = 0; i < 3; i++) {
      Serial.println(trustedRemoteIds[currRemoteIdx][i], HEX);
    }

    EEPROM.put(remoteEEPROMOffset, trustedRemoteIds[currRemoteIdx]);
    for (int i = 0; i < currRemoteIdx * 2; i++) {
      digitalWrite(LED_PIN, (i % 2) + 1 == 1);
      delay(75);
    }
    currRemoteIdx++;
  }
}

bool remoteIsTrusted() {
  for (int i = 0; i < 3; i++) {
    incomingRemoteId[i] = (uint8_t)incomingBuffer[5 + i];
  }
  return (memcmp(trustedRemoteIds[currRemoteIdx], incomingRemoteId,
                 sizeof(trustedRemoteIds[currRemoteIdx])) == 0);
}

SoftwareSerial rfSerial(A0, A1);
void setup() {
  Serial.begin(115200);
  rfSerial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LOCK_PIN, OUTPUT);
  pinMode(UNLOCK_PIN, OUTPUT);
  pinMode(PARKING_LIGHTS_PIN, OUTPUT);
  pinMode(HORN_PIN, OUTPUT);

  pinMode(PRGM_BTN_INTERRUPT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PRGM_BTN_INTERRUPT_PIN), prgmBtnISR,
                  CHANGE);

  for (int i = 0; i < MAX_REMOTES; i++) {
    EEPROM.get(remoteEEPROMOffset + i * 3, trustedRemoteIds[i]);
  }
}

void loop() {

  while (rfSerial.available() > 0) {
    readData = rfSerial.read();

    if (readData == '\r') {

      if (remoteIsTrusted()) {

        switch (incomingBuffer[3]) {
        case CMD_LOCK:
          Serial.println("[DEBUG] RECEIVED LOCK CMD");
          digitalWrite(LOCK_PIN, HIGH);
          digitalWrite(PARKING_LIGHTS_PIN, HIGH);
          delay(DOORLOCK_PULSE_MS);
          digitalWrite(LOCK_PIN, LOW);
          digitalWrite(PARKING_LIGHTS_PIN, LOW);
          delay(PARKING_LIGHTS_PULSE_MS);
          digitalWrite(PARKING_LIGHTS_PIN, HIGH);
          delay(PARKING_LIGHTS_PULSE_MS);
          digitalWrite(PARKING_LIGHTS_PIN, LOW);
          break;
        case CMD_UNLOCK:
          Serial.println("[DEBUG] RECEIVED UNLOCK CMD");
          digitalWrite(UNLOCK_PIN, HIGH);
          digitalWrite(PARKING_LIGHTS_PIN, HIGH);
          delay(DOORLOCK_PULSE_MS);
          digitalWrite(UNLOCK_PIN, LOW);
          digitalWrite(PARKING_LIGHTS_PIN, LOW);
          delay(PARKING_LIGHTS_PULSE_MS);
          digitalWrite(PARKING_LIGHTS_PIN, HIGH);
          delay(PARKING_LIGHTS_PULSE_MS);
          digitalWrite(PARKING_LIGHTS_PIN, LOW);
          break;
        default:
          Serial.println("[DEBUG] UNHANDLED CMD RECEIVED");
          break;
        }

      } else {
        Serial.println("[DEBUG] Remote is untrusted!");
        if (incomingBuffer[3] == CMD_LOCK) {
          saveRemoteAddressIfNeeded();
        }
      }

      bufferPos = 0;
      memset(incomingBuffer, 0x00, BUFFER_SIZE);
    } else if (bufferPos + 1 < BUFFER_SIZE) {
      incomingBuffer[bufferPos++] = readData;
    }
  }
}
