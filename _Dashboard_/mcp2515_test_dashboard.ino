#include <SPI.h>
#include <mcp2515.h>

MCP2515 mcp2515(53);

void setup() {
  Serial.begin(115200);

  SPI.begin();
  delay(200);   // important

  mcp2515.reset();
  delay(200);   // important

  Serial.println("Initializing CAN...");

  if (mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ) == MCP2515::ERROR_OK) {
    Serial.println("CAN Init OK");
  } else {
    Serial.println("CAN Init Failed");
  }

  mcp2515.setNormalMode();
}

void loop() {}
