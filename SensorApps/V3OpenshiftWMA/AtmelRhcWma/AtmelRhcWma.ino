//V2 OpenshiftWMA
#define F_CPU 4000000 // clock frequency, set according to clock used!
#include <EEPROMex.h>
#include <EEPROMVar.h>

#include <espduino.h>
#include <rest.h>
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

volatile long lastDebounceTime = 0;   // the last time the interrupt was triggered
#define debounceDelay 7    // the debounce time in ms; decrease if quick button presses are ignored, increase
//if you get noise (multipule button clicks detected by the code when you only pressed it once)

volatile boolean wifiConnected = false;
volatile boolean sendMessage = false;
String workingStatus = "\"alarmBoot\"";

volatile int counterTap = 0;
volatile int timeTap = 0;
volatile int counterDrup = 0;

volatile int countDelayDrup = 0;
volatile int druppingTimerAlarm = 0;
volatile boolean isDrupping = false;
volatile int countDelayTap;
volatile int countDelayLeak;
volatile byte alarm = 100;
volatile int numberOfBeeps = 0;
volatile boolean continousBeep = false;
volatile int countOnTime = 0;

volatile int timeHearbeat = 0;
volatile int literHearbeat = 0;

volatile int counterTemp = 0;
volatile int Temp = 0;
volatile boolean tempTrigger = false;
volatile int tempOffset = 330;


volatile boolean alarmWatertap = false;
volatile boolean alarmWaterleak = false;
volatile boolean alarmTemp = false;
volatile boolean alarmHeartbeat = false;
//volatile boolean alarmSimpleMessage = false;

volatile int value1 = 0;
volatile int value2 = 0;
volatile int nextLog = 450;
volatile byte keep_ADCSRA;

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
      counterTap++;
      counterDrup++;
      literHearbeat++;
      //delay(25);
      PCICR |= (1 << PCIE0);
      EIMSK |= (1 << PCINT0);
    if (alarm == 100) {
      alarm = 1;
    }
  }
}
ISR(WDT_vect)
{
    power_all_enable();
    countOnTime++;
    if (alarm == 100){
      asm volatile ("sbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTB)), "I" (PORTB5));
      delay(25);
      asm volatile ("cbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTB)), "I" (PORTB5));
      if (countOnTime > 40){
        alarm = 101;
      }
    }else{
    countDelayTap++;
    countDelayLeak++;
    timeHearbeat++;
    counterTemp++;
    if (countDelayTap >= 3) {        // 24sec voor water kraan open detectie
      countDelayTap = 0;
      if (counterTap != 0) {
        counterTap = 0;
        timeTap++;
        if (timeTap > 74) {
          alarmWatertap = true;     // 74 = 24min kraan vergeten
          timeTap = 0;
          alarm = 2;
        }
      }
      else {
        counterTap = 0;
        timeTap = 0;
        alarm = 0;
        asm volatile ("cbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTB)), "I" (PORTB1));
      }
    }

    if (counterDrup != 0) {      //Druppel detection
      counterDrup = 0;
      //countNoDrup = 0;
      if (countDelayLeak < 900) {
        isDrupping = true;
      }
      countDelayLeak = 0;
    }
    //Detectie droogte
    if (countDelayLeak >= 900) {   //2h droog
      countDelayLeak = 0;
      isDrupping = false;
    }
    // Druppel timer
    // bij start wacht op start waterloop
    if (isDrupping == false) {
      druppingTimerAlarm = 0;
      alarm = 0;
    }
    if (isDrupping == true) {
      druppingTimerAlarm++;
      if (druppingTimerAlarm >= 10800) { //10800 voor 24h (60 * 60 *24)/8
        alarmWaterleak = true;
        alarm = 3;
        druppingTimerAlarm = 0;
      }
    }
    if (timeHearbeat >= nextLog) {  //temp report every hour >=450
      timeHearbeat = 0;
      alarmHeartbeat = true;
      alarm = 4;
    }
    //temperatuur alarm
    if (counterTemp >= 20){
      counterTemp = 0;
      alarmTemp = true;
      alarm = 5;
    }
    }
    if (countOnTime > 50){
        countOnTime = 0;
        asm("wdr");
        WDTCSR |= (1 << WDCE) | (1 << WDE);
        WDTCSR &= ~(_BV(WDP3) | _BV(WDP0));
        WDTCSR |= _BV(WDE);
        WDTCSR &= ~(_BV(WDIE));
        while (1)
        {
        }
    }
}

int getTemp(void)
{
  unsigned int wADC;
  int t;
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN);  // enable the ADC
  delay(20);            // wait for voltages to become stable.
  ADCSRA |= _BV(ADSC);  // Start the ADC
  // Detect end-of-conversion
  while (bit_is_set(ADCSRA, ADSC));
  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCW;
  // The offset of 324.31 could be wrong. It is just an indication.
  t = (wADC - tempOffset )*100;// / 1.22;
  // The returned temperature is in degrees Celcius.
  return (t);
}
int readVcc()
{
  int result;
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX1);
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result;
  return result;
}
void setup() {
  int batVcc = readVcc();
  pinMode(Sensor_power, OUTPUT);
  digitalWrite(Sensor_power, LOW);
  pinMode(Esp_IO0, OUTPUT);//esp IO0
  digitalWrite(Esp_IO0, HIGH);//esp IO0
  
  pinMode(Trigger_switch, OUTPUT); //PB switch power low for Low power
  digitalWrite(Trigger_switch, LOW);
  pinMode(Step_EN, OUTPUT);
  digitalWrite(Step_EN, HIGH); //3.3v on
  pinMode(ADC_0, OUTPUT);
  digitalWrite(ADC_0, HIGH);

  digitalWrite(Step_EN, HIGH); //3V on

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

/*  for (int i = 255; i >= 128; i--) { //Turn on ESP-01
    analogWrite(10, i);
  }*/
  pinMode(Esp_power, OUTPUT);//PB Bridge
  digitalWrite(Esp_power, LOW);//PB Bridge
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

  int currentTemp;
  tempOffset = EEPROM.readInt(0);
  currentTemp = getTemp();
  currentTemp = getTemp();
  value2 = currentTemp;
  Serial.println("Used:" + String(literHearbeat >> 2) + "l" + " at " + String(currentTemp) + "C " + String(batVcc) + "mV");

  //Calibrate temperature
  pinMode(Trigger_switch, INPUT);
  if (digitalRead(Trigger_switch) == HIGH){
    calibrateTemp();
  }
  pinMode(Trigger_switch, OUTPUT); //PB switch power low for Low power
  digitalWrite(Trigger_switch, LOW);
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
void calibrateTemp()
{
  unsigned int wADC;
  unsigned int preTemp = 500;
  
  Serial.println("Calibrating Temperature");
  Serial.println("Stored value: " + String(EEPROM.readInt(0)) +  "C");
  while (digitalRead(Trigger_switch) == HIGH) {
  delay(5000);

  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN);  // enable the ADC
  delay(20);            // wait for voltages to become stable.
  ADCSRA |= _BV(ADSC);  // Start the ADC
  while (bit_is_set(ADCSRA, ADSC));
  wADC = ADCW;

   if (wADC < preTemp){
      numberOfBeeps = 2;  
    }
    preTemp = wADC;
    Serial.println(String(wADC));
  } 
  EEPROM.writeInt(0, wADC);
  tempOffset = wADC;
}

void sendRHCMessage()
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
    if (!rest.begin("wma-gonotify.rhcloud.com", 443, true)) {
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
    sprintf(data_buf, "\n{\"chipId\": \"#CHIPID#\",\"status\":%s,\"v1\":%d,\"v2\":%d}",workingStatus.c_str(),value1,value2);
    
    Serial.println(data_buf);
    rest.post("/wma", (const char*)data_buf);
    memset(data_buf, 0, sizeof(data_buf));
   if (rest.getResponse(data_buf, 256) == HTTP_STATUS_OK) {
      Serial.println(data_buf);
      char *ptr;  //temp pointer variable for pointer artihmetic
      char *fieldPtr; //temp pointer  variable for pointer artihmetic
      ptr=strstr(data_buf,"Nextlog:"); 
      if(ptr!=NULL){
          ptr=strstr(ptr," ");
          ptr++;
          fieldPtr=ptr;
          ptr=strstr(ptr,"s");
          *ptr='\0';
          Serial.println(fieldPtr);
          nextLog = atoi(fieldPtr)/8;
          Serial.println(nextLog);
      }
      
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
void software_Reboot()
{
  asm("wdr");
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  WDTCSR &= ~(_BV(WDP3) | _BV(WDP0));
  WDTCSR |= _BV(WDE);
  WDTCSR &= ~(_BV(WDIE));
  while (1)
  {
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
void loop() {

  if (sendMessage)
  {
    digitalWrite(Step_EN, HIGH); //3.3v on
    digitalWrite(Esp_IO0, HIGH);//esp IO0
    digitalWrite(Esp_RST, HIGH);
    //delay(100);
/*    for (int i = 255; i >= 128; i--) { //Turn on ESP-01
      analogWrite(10, i);
    }*/
    digitalWrite(Esp_power, LOW);//PB Bridge
    //delay(100);
    sendRHCMessage();
  }
  if (alarm == 6) {
    value1 = literHearbeat >> 2;
    int currentTemp;
    currentTemp = getTemp();
    currentTemp = getTemp();
    value2 = currentTemp;
    int batVcc = readVcc();
    workingStatus = "\"log\",\"v3\":" + String(batVcc);
    sendMessage = true;
    alarm = 0;
  }
  
  if (alarm == 0) {
    if ((millis() - lastDebounceTime) > debounceDelay) //if current time minus the last trigger time is greater than
    {
      asm volatile ("cbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTB)), "I" (PORTB5));
      goToSleep();
    }
  } else if (alarm == 1) {
    continousBeep = false;
    numberOfBeeps = 6;
    value1 = literHearbeat >> 2;
    int currentTemp;
    currentTemp = getTemp();
    currentTemp = getTemp();
    value2 = currentTemp;
    workingStatus = "\"alarmTrigger\"";
    alarm = 0;
    sendMessage = true;
  } else if (alarm == 101) {
    continousBeep = false;
    numberOfBeeps = 6;
    value1 = literHearbeat >> 2;
    int currentTemp;
    currentTemp = getTemp();
    currentTemp = getTemp();
    value2 = currentTemp;
    workingStatus = "\"alarmBoot\"";
    alarm = 0;
    sendMessage = true;

  } else if (alarmWatertap == true && sendMessage == false) {
    asm volatile ("sbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTB)), "I" (PORTB1));
    alarmWatertap = false;
    alarm = 0;
    value1 = literHearbeat >> 2;
    int currentTemp;
    currentTemp = getTemp();
    currentTemp = getTemp();
    value2 = currentTemp;
    workingStatus = "\"alarmWatertap\"";
    sendMessage = true;
   // Message = "#WRTL#";
  } else if (alarmWaterleak == true && sendMessage == false) {
    continousBeep = true;
    sendMessage = true;
    value1 = literHearbeat >> 2;
    int currentTemp;
    currentTemp = getTemp();
    currentTemp = getTemp();
    value2 = currentTemp;
    workingStatus = "\"alarmWaterleak\"";
    //Message = "#LEAK#";
  } else if (alarmHeartbeat == true && sendMessage == false) {
    sendMessage = true;
    value1 = literHearbeat >> 2;
    int currentTemp;
    currentTemp = getTemp();
    currentTemp = getTemp();
    value2 = currentTemp;
    int batVcc = readVcc();
    workingStatus = "\"log\",\"v3\":" + String(batVcc);
    alarmHeartbeat = false;
    alarm = 0;
  } else if (alarmTemp == true && sendMessage == false) {
    int currentTemp;
    currentTemp = getTemp();
    currentTemp = getTemp();
    Serial1.println(currentTemp, 1);
    if (currentTemp <= 600 && !tempTrigger) {
      sendMessage = true;
      value1 = literHearbeat >> 2;
      value2 = currentTemp;
      workingStatus = "\"alarmTemp\"";
      tempTrigger = true;
    } else if (currentTemp >= 800 && tempTrigger){
        tempTrigger = false;
    }
    alarmTemp = false;
    alarm = 0;
  }
}
