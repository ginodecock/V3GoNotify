//V2
#include "TimerOne.h"
#define LED 13
String inputString = "";  
boolean stringComplete = false;

volatile long lastDebounceTime = 0;   // the last time the interrupt was triggered
volatile int resetCounter = 0;
volatile boolean resetStat = true;
#define debounceDelay 7    // the debounce time in ms; decrease if quick button presses are ignored, increase
//if you get noise (multipule button clicks detected by the code when you only pressed it once)
#define LED 13
#define Esp_IO0 17
#define Esp_RST 16
#define Sensor_power 15
#define ADC_0 14
#define Esp_power 10
#define Buzzer 9
#define Step_EN 7
#define Trigger_switch 3
#define SDA 18
#define SCL 19

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
  pinMode(Trigger_switch, OUTPUT); //PB switch power low for Low power
  digitalWrite(Trigger_switch, LOW);
  pinMode(Step_EN, OUTPUT);
  digitalWrite(7, HIGH); //3.3v on
  pinMode(ADC_0, OUTPUT);
  digitalWrite(ADC_0, HIGH);
  digitalWrite(Step_EN, HIGH); //3V on
  pinMode(LED, OUTPUT);
  pinMode(9, OUTPUT);//buzzer
  digitalWrite(9, LOW);//buzzer
  pinMode(Esp_power, OUTPUT);//PB Bridge
  digitalWrite(Esp_power, LOW);//PB Bridge
  pinMode(Esp_RST, OUTPUT);//esp reset
  digitalWrite(Esp_RST, HIGH);//esp reset
  asm volatile ("cbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTC)), "I" (PORTC2));
  delay(10);
  asm volatile ("sbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTB)), "I" (PORTB5)); //groene led startup
  Serial.begin(38400);
  asm volatile ("sbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTC)), "I" (PORTC2));
  attachInterrupt(1, triggerDetection, CHANGE);
  digitalWrite(Buzzer, HIGH);
  delay(100);
  digitalWrite(Buzzer,LOW);
  Timer1.initialize(500000);
  Timer1.attachInterrupt(counterHandling);// attaches callback() as a timer overflow interrupt
  resetStat = false;
}
void counterHandling() // interrupt service routine that wraps a user defined function supplied by attachInterrupt
{
  if (!resetStat){
    resetCounter++;  
  }
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
      }else if (inputString.startsWith("received ping:")){
        Serial.println("-" + (String)resetCounter + "-");
        resetCounter = 0;
      }else{
        Serial.println(inputString);
      }
    inputString = "";
    stringComplete = false;
    }
  if (resetCounter > 100){
    resetStat = true;
    digitalWrite(Esp_power, HIGH);//PB Bridge 
    delay(400);
    digitalWrite(Esp_power, LOW);//PB Bridge 
    resetStat = false;
    resetCounter = 0;
  }
}
