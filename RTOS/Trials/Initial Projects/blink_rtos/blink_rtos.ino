#include <Arduino_FreeRTOS.h>
void blink(void*paramter);
void analogread(void*paramter1);
void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN,OUTPUT);
xTaskCreate(
  blink,
  "Blink",
  192,
  NULL,
  1,
  NULL
);
xTaskCreate(
  analogread,
  "Read",
  192,
  NULL,
  2,
  NULL
);
}

void loop() {
  
}
void blink(void*paramter){
  (void) paramter;
  digitalWrite(LED_BUILTIN,HIGH);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  digitalWrite(LED_BUILTIN,LOW);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
void analogread(void*paramter1){
  (void) paramter1;
  int value=analogRead(0);
  Serial.println(value);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}