
#include <Arduino.h>
#include <stdio.h>
#include <ESP8266WiFi.h>
extern "C" {
#include <espnow.h>
#include <user_interface.h>
#include <secPubSubClient.h>
#include <secNW_WatchDog.h>
#include <secATT_IOT.h>                         //AllThingsTalk IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type..

}

// Enter below your client credentials. 
// These credentials can be found in the configuration pane under your device in the smartliving.io website 

char deviceId[] = "<ATT device ID>"; // Your device id comes here
char clientId[] = "<ATT User ID>"; // Your client id comes here;
char clientKey[] = "ATT Client key>"; // Your client key comes here;

const char* ssid     = "<ACCESSPOINT>";
const char* password = "<Wifi password>";

ATTDevice Device(deviceId, clientId, clientKey);            //create the object that provides the connection to the cloud to manager the device.
char httpServer[] = "api.smartliving.io";                   // HTTP API Server host                  
const char* fingerprinthttp = "AE 03 8C 23 4C 36 1B 43 40 A7 E9 70 D9 48 B4 CE 1D 12 E3 12";
char mqttServer[] = "broker.smartliving.io";                // MQTT Server Address 
const char* fingerprintmqtt = "AE 03 8C 23 4C 36 1B 43 40 A7 E9 70 D9 48 B4 CE 1D 12 E3 12";

int alarm = 0;
int data1 = 0;
int counter = 0;
#define DELAY 10000
int temperature,humidity,battery,stat,soil;

#define WIFI_DEFAULT_CHANNEL 1  //Same as AP
uint8_t mac[] = {0x18, 0xFE, 0x34, 0xA2, 0x3C, 0x70};

//os_timer_t myTimer;
boolean tickOccured;
void timerCallback(void *pArg) {
  tickOccured = true;
}

void callback(char* topic, byte* payload, unsigned int length);
WiFiClientSecure ethClient;
PubSubClient pubSub(mqttServer, 8883, callback,ethClient);  
NW_WatchDog WatchDog(pubSub, deviceId, clientId, 10000); 

void printMacAddress(uint8_t* macaddr) {
  Serial.print("{");
  for (int i = 0; i < 6; i++) {
    Serial.print("0x");
    Serial.print(macaddr[i], HEX);
    if (i < 5) Serial.print(',');
  }
  Serial.println("}");
}
String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
void setup() {
  WiFi.disconnect();
  Serial.begin(38400);
  Serial.println("\nSoftware serial master");
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESPNowBridge","12345678",WIFI_DEFAULT_CHANNEL,0);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  uint8_t macaddr[6];
  wifi_get_macaddr(SOFTAP_IF, macaddr);
  Serial.print("mac address (SOFTAP_IF): ");
  printMacAddress(macaddr);

  if (esp_now_init() == 0) {
    Serial.println("init");
  } else {
    Serial.println("init failed");
    ESP.restart();
    return;
  }

  Serial.println("SET ROLE SLAVE");
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb([](uint8_t *macaddr, uint8_t *data, uint8_t len) {
    String sStat = getValue((char*)data,' ',0);
    String sTemperature = getValue((char*)data,' ',1);
    String sHumidity = getValue((char*)data,' ',2);
    String sBattery = getValue((char*)data,' ',3);
    String sSoil = getValue((char*)data,' ',4);
    stat = sStat.toInt();
    temperature = sTemperature.toInt();
    humidity = sHumidity.toInt();
    battery = sBattery.toInt();
    soil = sSoil.toInt();
    alarm = 1;
  });

  esp_now_register_send_cb([](uint8_t* macaddr, uint8_t status) {
    Serial.println("send_cb");

    Serial.print("mac address: ");
    printMacAddress(macaddr);

    Serial.print("status = "); Serial.println(status);
  });

  int res = esp_now_add_peer(mac, (uint8_t)ESP_NOW_ROLE_CONTROLLER, (uint8_t)WIFI_DEFAULT_CHANNEL, NULL, 0);

 // delay(1000);    
//  while(!Device.Connect(&ethClient, httpServer,443))                // connect the device with the IOT platform.
//    Serial.println("retrying");
//  Device.AddAsset(9, "YourDigitalActuatorname", "Digital Actuator Description", true, "boolean");   // Create the Digital Actuator asset for your device
  
  while(!Device.Subscribe(pubSub))                              // make certain that we can receive message from the iot platform (activate mqtt)
    Serial.println("retrying");  
}

void loop() {
  Device.Process();
   if(!WatchDog.CheckPing()){                                //if the ping failed, recreate the connection.  
    Serial.println("recreating broker connection");
    while(!Device.Subscribe(pubSub))                        // make certain that we can receive message from the iot platform (activate mqtt)
        Serial.println("retrying");
    WatchDog.Ping();                    //resstart the watchdog
  }
  if (alarm == 1){
    if (stat == 1){
      Serial.println(">1");
      Device.Send("true", 6);
      Serial.println("");  
      //WatchDog.Ping();
    }
    if (stat == 0){
      Serial.println(">0");
      Device.Send("false", 6);
      Serial.println("");
    }
    Device.Send(String(float(temperature)/10),1);
    Device.Send(String(float(humidity)/10),2);
    Device.Send(String(float(battery)/1000),3);
    Device.Send(String(float(soil)/10),4);
    alarm = 0;
  }
  yield();
}

// Callback function: handles messages that were sent from the iot platform to this device.
void callback(char* topic, byte* payload, unsigned int length) 
{ 
  String msgString; 
  {                                                           //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
      char message_buff[length + 1];                              //need to copy over the payload so that we can add a /0 terminator, this can then be wrapped inside a string for easy manipulation
      strncpy(message_buff, (char*)payload, length);              //copy over the data
      message_buff[length] = '\0';                                //make certain that it ends with a null     
      
      msgString = String(message_buff);
      msgString.toLowerCase();                                    //to make certain that our comparison later on works ok (it could be that a 'True' or 'False' was sent)
  }
  int* idOut = NULL;
  {               
      int pinNr = Device.GetPinNr(topic, strlen(topic));                                        
      Serial.println(msgString);
        if (msgString == "false") {
            Serial.println("<buttondetected");
        }
        if(!WatchDog.IsWatchDog(pinNr, msgString)){
        if(idOut != NULL)                                     //also let the iot platform know that the operation was succesful: give it some feedback. This also allows the iot to update the GUI's correctly & run scenarios.
          Device.Send(msgString, *idOut); 
      }
  }
}
