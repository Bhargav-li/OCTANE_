void setup() {

  TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11);  // Mode 10
  TCCR1A   = _BV(WGM13) | _BV(CS11);    
  ICR1 = 9;                                                    //
  OCR1A = 5;                                                   //d9
  
  pinMode(9, OUTPUT);  // Pin 3 (OC2A)
}

void loop() {

}
