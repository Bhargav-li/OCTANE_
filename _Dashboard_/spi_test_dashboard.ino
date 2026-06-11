#include <SPI.h>

void setup() {
  Serial.begin(115200);
  SPI.begin();
  Serial.println("SPI Started");
}

void loop() {
}
