#include <SoftwareSerial.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
extern "C" {
#include <espnow.h>
#include <user_interface.h>
}

#define WIFI_DEFAULT_CHANNEL 1

uint8_t esp1[] = {0x1A, 0xFE, 0x34, 0xA2, 0x46, 0x86};
uint8_t transmitStatus = 99;
uint8_t retry = 10;

void printMacAddress(uint8_t* macaddr) {
  Serial.print("{");
  for (int i = 0; i < 6; i++) {
    Serial.print("0x");
    Serial.print(macaddr[i], HEX);
    if (i < 5) Serial.print(',');
  }
  Serial.println("}");
}

void setup() {
  WiFi.disconnect();

  Serial.begin(38400);
  Serial.println("\nSoftware serial slave");

  WiFi.mode(WIFI_STA);
  //WiFi.softAP("ESPNowSlave","12345678",0,0);
  uint8_t macaddr[6];
  wifi_get_macaddr(STATION_IF, macaddr);
  Serial.print("STATION_IF: ");
  printMacAddress(macaddr);

  wifi_get_macaddr(SOFTAP_IF, macaddr);
  Serial.print("SOFTAP_IF: ");
  printMacAddress(macaddr);

  if (esp_now_init() == 0) {
    Serial.println("ok");
  } else {
    Serial.println("dl init failed");
    ESP.restart();
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_recv_cb([](uint8_t *macaddr, uint8_t *data, uint8_t len) {
    Serial.println("recv_cb");

    Serial.print("mac address: ");
    printMacAddress(macaddr);

    Serial.print("data: ");
    for (int i = 0; i < len; i++) {
      Serial.print(data[i], HEX);
    }
    Serial.print("  ");
  });
  esp_now_register_send_cb([](uint8_t* macaddr, uint8_t status) {
    Serial.print("status = "); Serial.println(status);
    transmitStatus = status;
    if (status == 1){
          Serial.println("RE");
    }
    if (status == 0){
          Serial.println("SL");
    }

   // Serial.print("status = "); Serial.println(status);
  });
  int res1 = esp_now_add_peer(esp1, (uint8_t)ESP_NOW_ROLE_SLAVE, WIFI_DEFAULT_CHANNEL, NULL, 0);

}

uint8_t len = 20;
uint8_t data[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t d2;
String str;

void loop() {

  while(Serial.available()) {
      str = Serial.readStringUntil('\n');
      Serial.println(str);
      
  }

 if (str != "") {
      str.trim();
      len = 20;
      uint8_t* datab = (uint8_t*)str.c_str();  
      esp_now_send(NULL, datab,len);
     str = "";
    }
}
