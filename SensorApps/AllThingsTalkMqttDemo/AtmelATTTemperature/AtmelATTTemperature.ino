//V3
#define F_CPU        4000000L
#define SERIAL_BUFFER_SIZE 256
#include <espduino.h>
#include <mqtt.h>
#include <rest.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <DallasTemperature4MHz.h>
#include <OneWire4MHz.h>

volatile long lastDebounceTime = 0;   // the last time the interrupt was triggered
#define debounceDelay 7    // the debounce time in ms; decrease if quick button presses are ignored, increase
//if you get noise (multipule button clicks detected by the code when you only pressed it once)
volatile byte alarm = 100;
volatile int triggerCounter = 0;
volatile int timeCounter = 0;
volatile boolean doCount = false;
volatile boolean doSetStatus = false;
volatile char data[32];
volatile char topic[128];
volatile int countPublished = 0;
volatile boolean chooseSend = false;

// Set these values according to your SmartLiving device
#define DEVICE_ID "----------------"
#define CLIENT_ID "-------"
#define CLIENT_KEY "----------"

// Tempsensor
#define ONE_WIRE_BUS 19
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Your WiFi credentials
#define MQTT_BROKER "api.allthingstalk.io"

// LED attached to pin 39
#define LED 13

ESP esp(&Serial1, &Serial, 6);
MQTT mqtt(&esp);
uint16_t _counter = 0;
boolean wifiConnected = false;

void wifiCb(void* response)
{
  uint32_t status;
  RESPONSE res(response);

  if (res.getArgc() == 1) {
    res.popArgs((uint8_t*)&status, 4);
    if (status == STATION_GOT_IP) {
      Serial.println("WIFI CONNECTED");

      // setup assets on platform
      //setupAssets();
      Serial.println("SL: Done setting assets");

      // connect to MQTT broker
      
      digitalWrite(9, HIGH);//buzzer
      delay(41);
      digitalWrite(9, LOW);
    //mqtt.connect(MQTT_BROKER, 1883,false);
    mqtt.connect(MQTT_BROKER, 8883,true);
      wifiConnected = true;
      doSetStatus = true;
    } else {
      wifiConnected = false;
      mqtt.disconnect();
    }

  }
}

void mqttConnected(void* response)
{
  char topic[128];
  sprintf(topic, "client.%s.in.device.%s.asset.*.command", CLIENT_ID, DEVICE_ID);
  Serial.println("Connected");
  mqtt.subscribe(topic);
}

void mqttDisconnected(void* response)
{
  Serial.println("Disconnected");
}

void mqttData(void* response)
{
  RESPONSE res(response);

  Serial.print("Received: topic=");
  String topic = res.popString();
  Serial.println(topic);

  Serial.print("data=");
  String data = res.popString();
  Serial.println(data);

  // we should check agains topic what asset is sending the data
  // but we have just one so we're skipping that
  //bool led = data == "true";
  if (data == "true"){
    bool currentState = digitalRead(LED);
    if (currentState){
      digitalWrite(LED, false); 
      doSetStatus = true;
    } 
    if (!currentState){
      digitalWrite(LED, true);  
      doSetStatus = true;  
    }  
  }
}

void mqttPublished(void* response)
{
  //digitalWrite(LED,false);
  //detectCrash = false;
  Serial.println("mqtt published");
  countPublished = 0;
}

void triggerDetection()
{
  if ((millis() - lastDebounceTime) > debounceDelay) //if current time minus the last trigger time is greater than
  { 
    lastDebounceTime = millis();
    if (alarm == 1){
      triggerCounter++;
      if (triggerCounter > 1){
         bool currentState = digitalRead(LED);
         if (currentState){
         digitalWrite(LED, false); 
         doSetStatus = true;
         } 
         if (!currentState){
         digitalWrite(LED, true);  
         doSetStatus = true;  
         }  
      triggerCounter = 0;
      }
    }
    if (alarm == 100){
      digitalWrite(LED, !digitalRead(LED));
      /*setup wifi*/
   esp.wifiConnect("#SSID#", "saved");
    WDTCSR |= _BV(WDIE);  
    alarm = 1;
    }
  }
}
ISR(WDT_vect)
{
  if (wifiConnected) {
     timeCounter++;
     if (timeCounter > 1){
        doCount = true;  
        chooseSend = !chooseSend;
        timeCounter = 0; 
     }
  }
  countPublished++;
}
void setup() {
  byte startupMode = EEPROM.read(0);
  EEPROM.write(0, 0);
  pinMode(17, OUTPUT);//esp IO0
  digitalWrite(17, HIGH);//esp IO0
  pinMode(LED, OUTPUT);
  pinMode(15, OUTPUT);
  digitalWrite(15, LOW);
  pinMode(3, OUTPUT); //PB switch power low for Low power
  digitalWrite(3, LOW);
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW); //3.3v off
  pinMode(14, OUTPUT);
  digitalWrite(14, HIGH);
  pinMode(LED, OUTPUT);
  pinMode(13, OUTPUT);//groene led
  pinMode(9, OUTPUT);//buzzer
  digitalWrite(9, LOW);//buzzer
  pinMode(10, OUTPUT);//PB Bridge
  digitalWrite(10, LOW);//PB Bridge
  pinMode(16, OUTPUT);//esp reset
  digitalWrite(16, HIGH);//esp reset

  //OneWire temp
  pinMode(15, OUTPUT);
  digitalWrite(15, LOW);
  sensors.begin();
  

  asm("wdr");
  MCUSR &= ~(1 << WDRF);
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  WDTCSR = 0x00;

  asm("wdr");
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  //WDTCSR = (1 << WDP3) | (1 << WDP0);
  WDTCSR = (1 << WDP2) | (1 << WDP1);

  
  asm volatile ("cbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTC)), "I" (PORTC2));
  delay(10);

  asm volatile ("sbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTB)), "I" (PORTB5)); //groene led startup
  //Serial.begin(19200,SERIAL_8E1);
  Serial1.begin(38400);
  Serial.begin(38400);
  if (startupMode == 0) {
    Serial.println("\nStart mode: normal");
  } else {
    Serial.println("\nStart mode: crash");
  }
  esp.enable();
     asm volatile ("sbi %0, %1 \n\t" :: "I" (_SFR_IO_ADDR(PORTC)), "I" (PORTC2));
  delay(500);
  esp.reset();
  delay(500);
  while (!esp.ready());
  char user[32];
  sprintf(user, "%s:%s", CLIENT_ID, CLIENT_ID);
  Serial.println("ARDUINO: setup mqtt client");
  if (!mqtt.begin("Go Notify", user, CLIENT_KEY, 100, 1)) {
    Serial.println("ARDUINO: fail to setup mqtt");
    while (1);
  }
Serial.print("ucsra:");
Serial.println(UCSR0A,HEX);
  /*setup mqtt events */
  mqtt.connectedCb.attach(&mqttConnected);
  mqtt.disconnectedCb.attach(&mqttDisconnected);
  mqtt.publishedCb.attach(&mqttPublished);
  mqtt.dataCb.attach(&mqttData);
  Serial.println("ARDUINO: setup wifi");
  esp.wifiCb.attach(&wifiCb);
  Serial.println("ARDUINO: system started");
  attachInterrupt(1, triggerDetection, CHANGE);
  attachInterrupt(0, triggerDetection, CHANGE);
  if (startupMode == 1){
    digitalWrite(LED, !digitalRead(LED));
      /*setup wifi*/
   esp.wifiConnect("#SSID#", "saved");
    WDTCSR |= _BV(WDIE);  
    alarm = 1;
    }
}
void setStatus(boolean newStatus) {
  char data[32];
  char topic[128];
  // payload format is timestamp|data. We can use 0 here, in that case
  // state update will be marked with server's time
  if (newStatus){
    sprintf(data, "0|true");  
  }
  if (!newStatus){
    sprintf(data, "0|false");  
  }
  // publish directly to Counter topic structure
  sprintf(topic, "client.%s.out.device.%s.asset.%s.state", CLIENT_ID, DEVICE_ID, "Status");
  mqtt.publish(topic, data);
}

void loop() {
  esp.process();
  //delay(20);
  if (doCount){
    if (chooseSend){
      sensors.requestTemperatures();
      sprintf(data, "0|%s", String(sensors.getTempCByIndex(0)).c_str());
      sprintf(topic, "client.%s.out.device.%s.asset.%s.state", CLIENT_ID, DEVICE_ID, "temperature");
      mqtt.publish(topic, data);
        
    } else {
      sprintf(data, "0|%d", _counter++);
      sprintf(topic, "client.%s.out.device.%s.asset.%s.state", CLIENT_ID, DEVICE_ID, "pulse");
      mqtt.publish(topic, data);
    }
    
    doCount = false;
  }
  if (doSetStatus){
    if (digitalRead(LED)){
      sprintf(data, "0|true");  
    }
    if (!digitalRead(LED)){
      sprintf(data, "0|false");  
    }
    sprintf(topic, "client.%s.out.device.%s.asset.%s.state", CLIENT_ID, DEVICE_ID, "Status");
    mqtt.publish(topic, data);

    setStatus(digitalRead(LED));
    doSetStatus = false;
  }
  if (countPublished > 24){
    EEPROM.write(0, 1);
    asm volatile ("  jmp 0");
  }
}
