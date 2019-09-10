
#define AT_CMD_CGDCONT "+CGDCONT"

#define IN_BUF_LEN 100
char incomingBuffer[IN_BUF_LEN];
int inBufIdx = 0;

void setup() {
  Serial.begin(115200);
}

void loop() {
  while(Serial.available()) {
    char inChar = Serial.read();
    if(inChar == '\n') {
      execSerialCmd();
      memset(incomingBuffer, '\0', IN_BUF_LEN);
      inBufIdx = 0;
    } else if (inBufIdx + 1 < IN_BUF_LEN - 1) {
      incomingBuffer[inBufIdx++] = inChar; 
    }
  }
}

void execSerialCmd() {
  int matchExcess = strcmp(incomingBuffer, "AT+CGDCONT");
  if(matchExcess >= 0) {
    delay(1500);
    Serial.println();
    Serial.println(AT_CMD_CGDCONT);
  } else {
    Serial.println();
    Serial.println("ERROR");
  }
}
