//V3
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <DHT12.h>
#include <Wire.h> 

#define LED 13
#define Esp_IO0 17
#define Esp_RST 16
#define Sensor_power 15
#define Soil_Sensor A0
#define Esp_power 10
#define Buzzer 9
#define Step_EN 7
#define Trigger_switch 3
#define SDA 18
#define SCL 19

String inputString = "";  
boolean stringComplete = false;
boolean stat = false;

volatile byte alarm = 1;
volatile byte keep_ADCSRA;
int humidity,temperature,battery,soil;

//dht12 DHT(0x5c);
DHT12 dht12; 
volatile long lastDebounceTime = 0;   // the last time the interrupt was triggered
#define debounceDelay 7    // the debounce time in ms; decrease if quick button presses are ignored, increase
//if you get noise (multipule button clicks detected by the code when you only pressed it once)

void triggerDetection()
{
 if (digitalRead(3) == 0){
  if ((millis() - lastDebounceTime) > debounceDelay) //if current time minus the last trigger time is greater than
  { //the delay (debounce) time, button is completley closed.
    lastDebounceTime = millis();
    alarm = 5;
    
  }
 }
}
ISR(WDT_vect)
{
    alarm = 4;
}

int readVcc()
{
  int result;
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX1);
  ADCSRA |= _BV(ADEN);  // Enable the ADC
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result;
  return result;
}
void dhtRead(){
  digitalWrite(Sensor_power, LOW);
  delay(10);
  temperature = dht12.readTemperature(CELSIUS) * 10;
  humidity = dht12.readHumidity() * 10;
  digitalWrite(Sensor_power, HIGH);
}
void soilRead(){
  digitalWrite(Soil_Sensor, INPUT_PULLUP);  // set pullup on analog pin 0 
  soil = analogRead(Soil_Sensor);
  digitalWrite(Soil_Sensor, LOW);
}
void setup() {
  pinMode(Esp_IO0, OUTPUT);//esp IO0
  digitalWrite(Esp_IO0, HIGH);//esp IO0
  pinMode(LED, OUTPUT);
  pinMode(Sensor_power, OUTPUT);
  digitalWrite(Sensor_power, LOW);
  pinMode(Soil_Sensor, INPUT);
  
  pinMode(Trigger_switch, OUTPUT); //PB switch power low for Low power
  digitalWrite(Trigger_switch, LOW);
  pinMode(Step_EN, OUTPUT);
  digitalWrite(Step_EN, HIGH); //3.3v on
   asm("wdr");
  MCUSR &= ~(1 << WDRF);
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  WDTCSR = 0x00;

  asm("wdr");
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  WDTCSR = (1 << WDP3) | (1 << WDP0);
  WDTCSR |= _BV(WDIE);
  
  pinMode(LED, OUTPUT);//groene led
  pinMode(Buzzer, OUTPUT);//buzzer
  digitalWrite(Buzzer, LOW);//buzzer
  pinMode(Esp_power, OUTPUT);//PB Bridge
  digitalWrite(Esp_power, LOW);//PB Bridge
  delay(10);
  pinMode(Esp_RST, OUTPUT);//esp reset
  digitalWrite(Esp_RST, HIGH);//esp reset
  asm volatile ("cbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTC)), "I" (PORTC2));
  delay(10);
  asm volatile ("sbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTB)), "I" (PORTB5)); //groene led startup
  Serial.begin(38400);
  Serial1.begin(38400);
  asm volatile ("sbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTC)), "I" (PORTC2));
  attachInterrupt(1, triggerDetection, CHANGE);
  tone(Buzzer,2000,100);

  Serial.println("Start");
  Wire.begin();
}

void serialEvent() {
  while (Serial1.available()) {
    char inChar = (char)Serial1.read();
    inputString += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}
const int bSize = 256;
char Buffer[bSize];  // Serial buffer


void loop() {
    serialEvent(); //call the function
    if (stringComplete){
      Serial.print(inputString);
      inputString.trim();
      if (inputString == "ok"){
        delay(50);
        stat = !stat;
        Serial1.print(String(stat) + " ");
        Serial1.print(String(temperature)+ " ");
        Serial1.print(String(humidity) + " ");
        Serial1.print(String(battery) + " ");
        Serial1.println(String(1023 - soil));
      
      }else if (inputString == "SL"){
        delay(50);
        alarm = 0;
        Serial.println("slapen");
      }else if (inputString == "RE"){
        Serial.println("opnieuw");
        digitalWrite(LED, !digitalRead(LED));
        //tone(Buzzer,2000,100);
        delay(50);
        alarm = 0;
      }
      inputString = "";
      stringComplete = false;
    };
    if (alarm == 5){
    power_all_enable();
    battery = readVcc();
    digitalWrite(Step_EN,HIGH);//3V on
    //dhtRead();
    //digitalWrite(Sensor_power, HIGH);
    digitalWrite(Esp_IO0, HIGH);//esp IO0
    delay(10);
    digitalWrite(Esp_power, LOW); //esp wifi
    digitalWrite(Esp_RST, HIGH);//esp reset
    alarm = 7;
  }
   if (alarm == 4){
    power_all_enable();
    battery = readVcc();
    digitalWrite(Step_EN,HIGH);//3V on
    dhtRead();
    soilRead();
    //digitalWrite(Sensor_power, HIGH);
    digitalWrite(Esp_IO0, HIGH);//esp IO0
    delay(10);
    digitalWrite(Esp_power, LOW); //esp wifi
    digitalWrite(Esp_RST, HIGH);//esp reset
    alarm = 7;
  }
  if (alarm ==0){
    goToSleep();
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
  keep_ADCSRA = ADCSRA;
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
  ADCSRA = keep_ADCSRA;
}


