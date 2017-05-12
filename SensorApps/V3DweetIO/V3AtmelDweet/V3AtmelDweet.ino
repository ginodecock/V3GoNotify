//V3
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>


#define LED 13
#define Esp_IO0 17
#define Esp_RST 16
#define Sensor_power 15
#define ADC_0 14
#define Esp_power 10
#define Buzzer 9
#define Step_EN 7
#define Trigger_switch 3

volatile byte alarm = 1;
volatile boolean wifiConnected = false;
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete


volatile boolean trigger = false;
volatile int numberOfBeeps = 0;
volatile boolean continousBeep = false;
volatile int retryWiFi;
volatile int timeCounter = 0;

void setup(){
  attachInterrupt(1, triggerDetection, CHANGE);
  attachInterrupt(0, triggerDetection, CHANGE);
  pinMode(Esp_IO0, OUTPUT);//esp IO0
  digitalWrite(Esp_IO0, HIGH);//esp IO0
  pinMode(LED, OUTPUT);
  pinMode(Sensor_power, OUTPUT);
  digitalWrite(Sensor_power, HIGH);
  pinMode(Trigger_switch, OUTPUT); //PB switch power low for Low power
  digitalWrite(Trigger_switch, LOW);
  pinMode(Step_EN, OUTPUT);
  digitalWrite(Step_EN, HIGH); //3.3v on
  pinMode(ADC_0, OUTPUT);
  digitalWrite(ADC_0, HIGH);
  digitalWrite(Step_EN, HIGH); //3V on
  pinMode(LED, OUTPUT);//groene led
  pinMode(Buzzer, OUTPUT);//buzzer
  digitalWrite(Buzzer, LOW);//buzzer
  pinMode(Esp_power, OUTPUT);//PB Bridge
  digitalWrite(Esp_power, LOW);//PB Bridge
  delay(10);
  pinMode(Esp_RST, OUTPUT);//esp reset
  digitalWrite(Esp_RST, HIGH);//esp reset
  digitalWrite(LED,HIGH);
  Serial1.begin(38400);
  Serial.begin(38400);
  delay(1000);
  digitalWrite(LED,LOW);
  Serial.println("START");
}
void triggerDetection()
{
  //Serial.println("Trigger");
  digitalWrite(LED, HIGH); 
  if (alarm <= 1){
    power_all_enable();
    alarm = 5;
  }
}
void loop(){
  if (alarm == 5){
        digitalWrite(Step_EN,HIGH);//3V on
        digitalWrite(Sensor_power, LOW);
        digitalWrite(LED, HIGH);//
        delay(1000);
        digitalWrite(Esp_power, LOW); //esp wifi
        digitalWrite(Esp_IO0, HIGH);//esp IO0
        digitalWrite(Esp_RST, HIGH);
        delay(1500); //wait for the sensor to be ready 
  }
  Serial1Event();
  if (stringComplete){
    Serial.print(inputString);
    inputString.trim();
    if (inputString.startsWith("[HTTP] POST... code: 200")){
        Serial.println("Dweet OK");
        digitalWrite(LED, LOW);
        alarm = 0;
    }
    inputString = "";
    stringComplete = false;
  }
  if (alarm ==0){
    wifiConnected = false;
    goToSleep();
  }
}

void Serial1Event() {
  while (Serial1.available()) {
    char inChar = (char)Serial1.read();
    inputString += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}
void goToSleep(void)
{
  digitalWrite(LED,LOW);
  digitalWrite(Step_EN, LOW);//3V off
  digitalWrite(Esp_power, HIGH);
  digitalWrite(Esp_RST, LOW);
  digitalWrite(Esp_IO0, LOW);//esp IO0
  asm volatile ("cbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTD)), "I" (PORTD7));// 3.3 v off
 // keep_ADCSRA = ADCSRA;
  ADCSRA = 0;

  // turn off various modules
  power_all_disable();

  set_sleep_mode (SLEEP_MODE_PWR_DOWN);
  noInterrupts ();
  sleep_enable();

  // turn off brown-out enable in software
   MCUCR = bit (BODS) | bit (BODSE);
   MCUCR = bit (BODS);
  interrupts ();             // guarantees next instruction executed
  sleep_cpu ();
  sleep_disable();
//  ADCSRA = keep_ADCSRA;
}


