//V2
#include <SFE_BMP180.h>
#include <Wire.h>
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
volatile byte keep_ADCSRA;
volatile boolean wifiConnected = false;
double batVcc = 0;
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete


volatile int L_TEMP;
volatile int H_TEMP;
volatile double currentTemp;
volatile boolean trigger = false;
volatile int numberOfBeeps = 0;
volatile boolean continousBeep = false;
volatile int retryWiFi;
volatile int timeCounter = 0;

void setup(){
  enableWatchdog();
  attachInterrupt(1, triggerDetection, CHANGE);
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
  //Timer1 for buzzer
  cli();          // disable global interrupts
  TCCR1A = 0;     // set entire TCCR1A register to 0
  TCCR1B = 0;     // same for TCCR1B
  OCR1A = 3906;
  // turn on CTC mode:
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler:
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS12);
  // enable timer compare interrupt:
  TIMSK1 |= (1 << OCIE1A);
  sei();


  Serial1.begin(38400);
  Serial.begin(38400);
  Serial.println("REBOOT");
  
  inputString.reserve(200);
  
  delay(1500); //wait for the sensor to be ready 
  sendCommand(">ConfigWiFi");
}
void triggerDetection()
{
  if (alarm <= 1){
    power_all_enable();
    alarm = 5;
  }
}
ISR(WDT_vect)
{
  timeCounter++;
  if (timeCounter > 100){
     if (alarm == 0){
        power_all_enable();
        alarm = 2;    
      }
      timeCounter = 0; 
   }
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
void loop(){
  // BMP180
char status;
double T,P;
  if (alarm == 2){


        batVcc = readVcc();
        digitalWrite(Step_EN,HIGH);//3V on
        SFE_BMP180 pressure;
        digitalWrite(Sensor_power, LOW);
        pressure.begin();
  
        digitalWrite(LED, HIGH);//
        delay(1000);
        Serial.println("\n----BMP180 handling----");
        digitalWrite(Esp_power, LOW); //esp wifi
        digitalWrite(Esp_IO0, HIGH);//esp IO0
        digitalWrite(Esp_RST, HIGH);
        delay(1500); //wait for the sensor to be ready 
        sendCommand(">ConfigWiFi");
        delay(3000);
        
        status = pressure.startTemperature();
        if (status != 0)
        {
          delay(status);
          status = pressure.getTemperature(T);
          if (status != 0)
          {
            Serial.print("temperature: ");
            Serial.print((9.0/5.0)*T+32.0,2);
            Serial.println(" deg F");
            status = pressure.startPressure(3);
            if (status != 0)
            {
              delay(status);
              status = pressure.getPressure(P,T);
              if (status != 0)
              {
              // Print out the measurement:
              Serial.print("absolute pressure: ");
              Serial.print(P*0.0295333727,2);
              Serial.println(" inHg");
              }
              else Serial.println("error retrieving pressure measurement\n");
            }
            else Serial.println("error starting pressure measurement\n");
          }
          else Serial.println("error retrieving temperature measurement\n");
        }
        else Serial.println("error starting temperature measurement\n");
        sendCommand(">value1=tempf=" + String((9.0/5.0)*T+32.0,2));
        delay(100);
        sendCommand(">value2=baromin=" + String(P*0.0295333727,2));
        delay(100);
        alarm = 6;
        sendCommand(">Push");
  }
  
  // Button
  if (alarm == 5){
    batVcc = readVcc();
    digitalWrite(Step_EN,HIGH);//3V on
    SFE_BMP180 pressure;
    digitalWrite(Sensor_power, LOW);
    pressure.begin();
    digitalWrite(LED, HIGH);//
    delay(1000);
    Serial.println("\n----Button handling----");
    digitalWrite(Esp_power, LOW); //esp wifi
    digitalWrite(Esp_IO0, HIGH);//esp IO0
    digitalWrite(Esp_RST, HIGH);
    //wifiConnected = false;
    delay(1500); //wait for the sensor to be ready 
    sendCommand(">ConfigWiFi");
    delay(3000);
    status = pressure.startTemperature();
        if (status != 0)
        {
          delay(status);
          status = pressure.getTemperature(T);
          if (status != 0)
          {
            Serial.print("temperature: ");
            Serial.print((9.0/5.0)*T+32.0,2);
            Serial.println(" deg F");
            status = pressure.startPressure(3);
            if (status != 0)
            {
              delay(status);
              status = pressure.getPressure(P,T);
              if (status != 0)
              {
              // Print out the measurement:
              Serial.print("absolute pressure: ");
              Serial.print(P*0.0295333727,2);
              Serial.println(" inHg");
              }
              else Serial.println("error retrieving pressure measurement\n");
            }
            else Serial.println("error starting pressure measurement\n");
          }
          else Serial.println("error retrieving temperature measurement\n");
        }
        else Serial.println("error starting temperature measurement\n");
    sendCommand(">value1=tempf=" + String((9.0/5.0)*T+32.0,2));
    delay(100);
    sendCommand(">value2=baromin=" + String(P*0.0295333727,2));
    delay(100);
    alarm = 6;
    sendCommand(">Push");
  }

    // Command handler
  Serial1Event(); //call the function
  if (stringComplete) {
    Serial.print(inputString);
    // clear the string:
    if (stringComplete == true) {
      //Serial1.println("+");
      inputString.trim();
      if (inputString == "<WiFi_Connected"){
        Serial.println("WiFi connected detected");
        wifiConnected = true;
        retryWiFi =0;
      }
      if (inputString == "<Done_Sending"){
        Serial.println("Done sending detected");
        alarm = 0;
      }
      if (inputString.startsWith("<L_TEMP=")){
        Serial.print("Low Temp: ");
        L_TEMP = inputString.substring(8).toInt();
        Serial.println(L_TEMP); 
      }
      if (inputString.startsWith("<H_TEMP=")){
        Serial.print("High Temp: ");
        H_TEMP = inputString.substring(8).toInt();
        Serial.println(H_TEMP); 
      }
      if (inputString == "<No_Config"){
        Serial.println("No Config detected");
        numberOfBeeps = 2;
      }
      if (inputString == "<No_WiFi_Connection"){
        Serial.println("No Wifi connection detected");
        numberOfBeeps = 4;
        if (retryWiFi >= 1){
          retryWiFi--;
          alarm = 5;
        }
      }
      if (inputString == "<No_Http_Connection"){
        Serial.println("No HTTP connection detected");
        numberOfBeeps = 6;
      }
    }
    inputString = "";
    stringComplete = false;
  }

  //Power save
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

void sendCommand(String toExecute){
  long nextUpdate = millis() + 1000;
  int retry = 0; 
  char inChar;
  while (retry <= 3){
    Serial1.println(toExecute);
    Serial.println(toExecute);
    while (inChar != '<'){
      while (Serial1.available()) {
        inChar = (char)Serial1.read();
      }
      if (millis() >= nextUpdate){
        Serial.println("Com Timeout");
        retry++;
      }  
    }
    //Serial1.println("Command received");
    retry = 100;
  }
}

void enableWatchdog()
{
  cli();
  /*** Setup the WDT ***/
  
  /* Clear the reset flag. */
  MCUSR &= ~(1<<WDRF);
  
  /* In order to change WDE or the prescaler, we need to
   * set WDCE (This will allow updates for 4 clock cycles).
   */
  WDTCSR |= (1<<WDCE) | (1<<WDE);

  /* set new watchdog timeout prescaler value */
  WDTCSR = 1<<WDP0 | 1<<WDP3; /* 8.0 seconds */
  //WDTCSR = 1<<WDP3 | 1<<WDP2; /* 8.0 seconds */
  
  /* Enable the WD interrupt (note no reset). */
  WDTCSR |= _BV(WDIE);
  sei();
}
/* disables the watchdog timer */
void disableWatchdog()
{
  cli();
  wdt_reset();
  MCUSR &= ~(1<<WDRF);
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  WDTCSR = 0x00;
  sei();
}

void goToSleep(void)
{
  digitalWrite(Sensor_power, HIGH);
  digitalWrite(Step_EN, LOW);//3V off
  digitalWrite(Esp_power, HIGH);
  digitalWrite(Esp_RST, LOW);
  digitalWrite(Esp_IO0, LOW);//esp IO0
  digitalWrite(LED, LOW);
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

double readVcc()
{
  long result;
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX1);
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result;
  return result;
}

