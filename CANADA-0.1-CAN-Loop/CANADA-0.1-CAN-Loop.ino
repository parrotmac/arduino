#include <SPI.h>
#include <mcp2515.h>

int blinkLED = 13;

struct can_frame canMsg1;
struct can_frame canMsg2;
MCP2515 mcp2515(9);


void setup() {
  canMsg1.can_id  = 0x060;
  canMsg1.can_dlc = 8;
  canMsg1.data[0] = 0x01;
  canMsg1.data[1] = 0x02;
  canMsg1.data[2] = 0x03;
  canMsg1.data[3] = 0x04;
  canMsg1.data[4] = 0x05;
  canMsg1.data[5] = 0x06;
  canMsg1.data[6] = 0x07;
  canMsg1.data[7] = 0x08;

  canMsg2.can_id  = 0x065;
  canMsg2.can_dlc = 8;
  canMsg2.data[0] = 0x08;
  canMsg2.data[1] = 0x07;
  canMsg2.data[2] = 0x06;
  canMsg2.data[3] = 0x05;
  canMsg2.data[4] = 0x04;
  canMsg2.data[5] = 0x03;
  canMsg2.data[6] = 0x02;
  canMsg2.data[7] = 0x01;
  
  Serial1.begin(115200);
  SPI.begin();
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_16MHZ);
  mcp2515.setNormalMode();

  pinMode(blinkLED, OUTPUT);
  Serial1.println("Hello from CANADA1 [Made in California]");
}

void loop() {

  digitalWrite(blinkLED, HIGH);
  Serial1.println("Sending message 1");
  mcp2515.sendMessage(&canMsg1); 
  
  delay(500);

  digitalWrite(blinkLED, LOW);
  Serial1.println("Sending message 2");
  mcp2515.sendMessage(&canMsg2); 
  
  delay(500);

}
