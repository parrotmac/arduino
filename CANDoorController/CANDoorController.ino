#include <SPI.h>
#include <mcp2515.h>

struct can_frame canMsg1;
struct can_frame canMsg2;
MCP2515 mcp2515(9);


void setup() {

//  canMsg1.can_id  = 0x074;
//  canMsg1.can_dlc = 8;
//  canMsg1.data[0] = 0x00;
//  canMsg1.data[1] = 0x05;
//  canMsg1.data[2] = 0x32;
//  canMsg1.data[3] = 0xFA;
//  canMsg1.data[4] = 0x26;
//  canMsg1.data[5] = 0x8E;
//  canMsg1.data[6] = 0xBE;
//  canMsg1.data[7] = 0x86;
//
//  canMsg2.can_id  = 0x074;
//  canMsg2.can_dlc = 8;
//  canMsg2.data[0] = 0x03;
//  canMsg2.data[1] = 0x06;
//  canMsg2.data[2] = 0x00;
//  canMsg2.data[3] = 0x08;
//  canMsg2.data[4] = 0x01;
//  canMsg2.data[5] = 0x00;
//  canMsg2.data[6] = 0x00;
//  canMsg2.data[7] = 0xA0;
//  
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
  
  while (!Serial);
  Serial.begin(115200);
  SPI.begin();
  
  mcp2515.reset();
//  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setBitrate(CAN_500KBPS, MCP_16MHZ);
  mcp2515.setNormalMode();
  
  Serial.println("Example: Write to CAN");
}

void sendLock() {
  mcp2515.sendMessage(&canMsg1); 
}

void sendUnlock() {
  mcp2515.sendMessage(&canMsg2); 
}

void loop() {
  
  sendLock();
  
  delay(500);

  sendUnlock();
  
  delay(500);

}
