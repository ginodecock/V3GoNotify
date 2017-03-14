//V3 IFTTT PIR people detect
#define F_CPU 4000000 // clock frequency, set according to clock used!
#include <espduino.h>
#include <rest.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include "PinChangeInterrupt.h"

#define LED 13
#define Esp_IO0 17
#define Esp_RST 16
#define Sensor_power 15
#define Esp_power 10
#define Buzzer 9
#define Step_EN 7
#define Trigger_switch 3
#define PIR_MOTION_SENSOR 18
#define IFTTT_trigger "/trigger/<Ifttt event>/with/key/<Ifttt key>"

volatile long lastDebounceTime = 0;   // the last time the interrupt was triggered
#define debounceDelay 7    // the debounce time in ms; decrease if quick button presses are ignored, increase
//if you get noise (multipule button clicks detected by the code when you only pressed it once)

volatile boolean wifiConnected = false;
volatile boolean sendMessage = false;

volatile byte alarm = 100;
volatile int numberOfBeeps = 0;
volatile boolean continousBeep = false;

void wifiCb(void* response)
{
  uint32_t status1;
  RESPONSE res(response);
  Serial.print(F("getArcWIFI CON STAT: "));
  Serial.println(res.getArgc());

  if (res.getArgc() == 1) {
    res.popArgs((uint8_t*)&status1, 4);
    Serial.print(F("WIFI CON STAT: "));
    Serial.println(status1);

    if (status1 == STATION_GOT_IP) {
      Serial.println(F("WIFI CONNECTED"));
      Serial.println(status1);

      wifiConnected = true;
    } else {
      wifiConnected = false;
    }
  }
}
void triggerDetection()
{
  if ((millis() - lastDebounceTime) > debounceDelay) //if current time minus the last trigger time is greater than
  { //the delay (debounce) time, button is completley closed.
    lastDebounceTime = millis();
    power_all_enable();
    asm volatile ("sbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTB)), "I" (PORTB5));
      PCICR |= (0 << PCIE0);
      EIMSK |= (0 << PCINT0);
      alarm = 1;
      //delay(25);
      PCICR |= (1 << PCIE0);
      EIMSK |= (1 << PCINT0);
    if (alarm == 100) {
      alarm = 1;
    }
  }
}
void pirDetection()
{
  if (alarm == 0){
        disablePinChangeInterrupt(digitalPinToPinChangeInterrupt(PIR_MOTION_SENSOR));
        digitalWrite(LED,HIGH);
        alarm = 1;
        digitalWrite(Sensor_power, HIGH);
    }
}
ISR(WDT_vect)
{
    power_all_enable();
}

void setup() {
  pinMode(Sensor_power, OUTPUT);
  digitalWrite(Sensor_power, LOW);
  pinMode(Esp_IO0, OUTPUT);//esp IO0
  digitalWrite(Esp_IO0, HIGH);//esp IO0
  
  pinMode(Trigger_switch, OUTPUT); //PB switch power low for Low power
  digitalWrite(Trigger_switch, LOW);
  pinMode(Step_EN, OUTPUT);
  digitalWrite(Step_EN, HIGH); //3.3v on

  pinMode(LED, OUTPUT);//groene led
  pinMode(Buzzer, OUTPUT);//buzzer
  digitalWrite(Buzzer, LOW);//buzzer

  pinMode(Esp_power, OUTPUT);//PB Bridge
  digitalWrite(Esp_power, HIGH);//PB Bridge
  pinMode(Esp_RST, OUTPUT);//esp reset
  digitalWrite(Esp_RST, HIGH);//esp reset

  asm volatile ("sbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTB)), "I" (PORTB5)); //groene led startup
  //Timer1 for buzzer
  cli();          // disable global interrupts
  TCCR1A = 0;     // set entire TCCR1A register to 0
  TCCR1B = 0;     // same for TCCR1B
  OCR1A = 500;//3906;
  // turn on CTC mode:
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler:
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS12);
  // enable timer compare interrupt:
  TIMSK1 |= (1 << OCIE1A);
  sei();

  Serial.begin(38400);
  Serial1.begin(38400);
  Serial.println("start v3.0");
  attachInterrupt(0, triggerDetection, CHANGE);
  attachInterrupt(1, triggerDetection, CHANGE);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(PIR_MOTION_SENSOR), pirDetection, CHANGE);
  disablePinChangeInterrupt(digitalPinToPinChangeInterrupt(PIR_MOTION_SENSOR));
  tone(Buzzer,2000,100);
}
ISR(TIMER1_COMPA_vect)
{
  if (!continousBeep) {
    if (numberOfBeeps > 0)
    {
      tone(Buzzer,2000,100);
      numberOfBeeps--;
    }
  } else {
    tone(Buzzer,2000,100);
  }
}

void sendIFTTTMessage()
{
  int retrySend = 0;
  do
  {
    char data_buf[256];
    //reset wifi
    asm volatile ("cbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTC)), "I" (PORTC2));
 
    ESP esp(&Serial1, &Serial, 6);
    REST rest(&esp);

    wifiConnected = false;
    esp.enable();
    asm volatile ("sbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTC)), "I" (PORTC2));
    
    while (!esp.ready());
    if (!rest.begin("maker.ifttt.com", 80, false)) {
      Serial.println("GN: failed setup rest client");
      while (1);
    }
    rest.setContentType("application/json");
    rest.setHeader("Authorization: GDC\r\n");
    Serial.println("GN: setup wifi");
    esp.wifiCb.attach(&wifiCb);
    esp.wifiConnect("#SSID#", "saved");
    int timeout = 0;
    while (!wifiConnected)
    {
      esp.process();
      delay(10);
      timeout++;
      if (timeout > 3000) break;
    }
    if (!wifiConnected) {
      asm volatile ("cbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTB)), "I" (PORTB5));
      delay(50);              // wait for a second
      asm volatile ("sbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTB)), "I" (PORTB5));
      delay(50);
    }
    memset(data_buf, 0, sizeof(data_buf));
    sprintf(data_buf, "\n{\"value1\": \"#CHIPID#\"}");
    
    Serial.println(data_buf);
    rest.post(IFTTT_trigger, (const char*)data_buf);
    memset(data_buf, 0, sizeof(data_buf));
   if (rest.getResponse(data_buf, 256) == HTTP_STATUS_OK) {
      Serial.println(data_buf);
      Serial.println(F("GN: POST successful"));
      asm volatile ("cbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTB)), "I" (PORTB5));
      delay(50);              // wait for a second
      asm volatile ("sbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTB)), "I" (PORTB5));
      delay(50);
      retrySend = 100;
    } else {
      Serial.println(data_buf);
      Serial.println(F("GN: POST error"));
      retrySend++;
      if (alarm == 100){
        retrySend = 100;
        esp.disable();
        esp.reset();
       } //Startup try once
    }
  } while (retrySend < 3);
  sendMessage = false;
}
void goToSleep(void)
{
  digitalWrite(LED,LOW);
  digitalWrite(Step_EN, LOW);//3V off
  digitalWrite(Esp_power, HIGH);
  digitalWrite(Esp_RST, LOW);
  digitalWrite(Esp_IO0, LOW);//esp IO0
  asm volatile ("cbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTD)), "I" (PORTD7));// 3.3 v off

  // turn off various modules
  //power_all_disable();
  enablePinChangeInterrupt(digitalPinToPinChangeInterrupt(PIR_MOTION_SENSOR));
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);
  noInterrupts ();
  sleep_enable();

  // turn off brown-out enable in software
   MCUCR = bit (BODS) | bit (BODSE);
   MCUCR = bit (BODS);
  interrupts ();             // guarantees next instruction executed
  sleep_cpu ();
  sleep_disable();
}
void loop() {
  //Serial.println(alarm);
  if (sendMessage)
  {
    digitalWrite(Step_EN, HIGH); //3.3v on
    delay(100);
    digitalWrite(Esp_IO0, HIGH);//esp IO0
    digitalWrite(Esp_power, LOW);//PB Bridge

    delay(100);
    sendIFTTTMessage();
    alarm = 0;
  }
  if (alarm == 0) {
    if ((millis() - lastDebounceTime) > debounceDelay) //if current time minus the last trigger time is greater than
    {
      digitalWrite(LED, LOW);
      digitalWrite(Esp_power, HIGH);//PB Bridge
      digitalWrite(Sensor_power, LOW);
      delay(10000);
      
      goToSleep();
    }
  } else if (alarm == 1) {
    continousBeep = false;
    numberOfBeeps = 1;
    
    sendMessage = true;
  } 
}
