
void setup() {
    Serial.begin(9600);
      pinMode(10,OUTPUT);
      pinMode(A1,INPUT);      
}

void loop() {
    int a = analogRead(A1);
    if(0<=a &&a<=511){
      digitalWrite(10, LOW);
      Serial.println(a);
      Serial.println("da");
      }
      
    if(511<=a&&a<=1023){
      digitalWrite(10, HIGH);
      Serial.println(a);
      Serial.println("ad");
      }
      delay(200);         
}
