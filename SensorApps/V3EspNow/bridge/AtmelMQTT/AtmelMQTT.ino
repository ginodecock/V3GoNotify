//V2
// LED attached to pin 39
#define LED 13
String inputString = "";  
boolean stringComplete = false;

volatile long lastDebounceTime = 0;   // the last time the interrupt was triggered
#define debounceDelay 7    // the debounce time in ms; decrease if quick button presses are ignored, increase
//if you get noise (multipule button clicks detected by the code when you only pressed it once)

void triggerDetection()
{
 if (digitalRead(3) == 0){
  if ((millis() - lastDebounceTime) > debounceDelay) //if current time minus the last trigger time is greater than
  { //the delay (debounce) time, button is completley closed.
    lastDebounceTime = millis();
    digitalWrite(13, !digitalRead(13)); 
    Serial.println(">" + String(digitalRead(13)));
  }
 }
}
void setup() {
  pinMode(LED, OUTPUT);
  pinMode(15, OUTPUT);
  digitalWrite(15, LOW);
  pinMode(3, OUTPUT); //PB switch power low for Low power
  digitalWrite(3, LOW);
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH); //3.3v on
  pinMode(14, OUTPUT);
  digitalWrite(14, HIGH);
  digitalWrite(7, HIGH); //3V on
  pinMode(LED, OUTPUT);
  pinMode(13, OUTPUT);//groene led
  pinMode(9, OUTPUT);//buzzer
  digitalWrite(9, LOW);//buzzer
  pinMode(10, OUTPUT);//PB Bridge
  digitalWrite(10, LOW);//PB Bridge
  pinMode(16, OUTPUT);//esp reset
  digitalWrite(16, HIGH);//esp reset
  asm volatile ("cbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTC)), "I" (PORTC2));
  delay(10);
  asm volatile ("sbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTB)), "I" (PORTB5)); //groene led startup
  Serial.begin(38400);
  asm volatile ("sbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTC)), "I" (PORTC2));
  attachInterrupt(1, triggerDetection, CHANGE);
  digitalWrite(9, HIGH);
        delay(100);
        digitalWrite(9,LOW);
}
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}
void loop() {
    serialEvent(); //call the function
    if (stringComplete == true) {
      inputString.trim();
      if (inputString.startsWith(">1")){
        digitalWrite(13, true);
        Serial.println(inputString);
      }else if (inputString.startsWith(">0")){
        digitalWrite(13, false);
        Serial.println(inputString);
      }else{
      Serial.println(inputString);
      }
    inputString = "";
    stringComplete = false;
    }
}
