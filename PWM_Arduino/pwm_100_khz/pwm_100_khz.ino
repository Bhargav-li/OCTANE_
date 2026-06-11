#include<TimerOne.h>
void setup() {
  // put your setup code here, to run once:
pinMode(9,OUTPUT);
Timer1.initialize(10);
Timer1.pwm(9,512);
}

void loop() {
  // put your main code here, to run repeatedly:

}
