#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

ESP8266WiFiMulti WiFiMulti;

HTTPClient http;
char payload[30];
char url[128];

const char* ssid = "Wifi AP SSID";
const char* password = "Wifi password";
const char* thingName = "GoNotify";

void setup() {
    Serial.begin(38400);
    Serial.println();
    Serial.println();
    Serial.println();

    for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("[SETUP] WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }

    WiFiMulti.addAP(ssid, password);

    http.setReuse(true);
    sprintf(url, "http://dweet.io/dweet/for/%s", thingName);
}

void loop() {
    // wait for WiFi connection
    if((WiFiMulti.run() == WL_CONNECTED)) {
        http.begin(url);
        http.addHeader("content-type", "application/json");
        
        sprintf(payload, "\r\n{\"value\":%d}\r\n", millis());

        int httpCode = http.POST(payload);

        if(httpCode > 0) {
            Serial.printf("\n[HTTP] POST... code: %d\n", httpCode);

            if(httpCode == HTTP_CODE_OK) {
                http.writeToStream(&Serial);
            }
        } else {
            Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
    }
    
    delay(3000); // respect dweet.io rate control.
}



