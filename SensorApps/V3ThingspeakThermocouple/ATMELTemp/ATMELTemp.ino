#include <max6675.h>

#define LED 13
#define Esp_IO0 17
#define Esp_RST 16
#define Sensor_power 15
#define ADC_0 14
#define Esp_power 10
#define Buzzer 9
#define Step_EN 7
#define Trigger_switch 3

int thermoDO = 2;
int thermoCS = 19;
int thermoCLK = 18;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

#define SSID "WifiAP"
#define PASS "WifiPassword"
#define IP "184.106.153.149" // thingspeak.com
String GET = "GET /update?key=<Thingspeak Key>&field1=";


void setup()
{
  pinMode(Esp_IO0, OUTPUT);//esp IO0
  digitalWrite(Esp_IO0, HIGH);//esp IO0
  pinMode(LED, OUTPUT);
  pinMode(Sensor_power, OUTPUT);
  digitalWrite(Sensor_power, LOW);
  pinMode(Trigger_switch, OUTPUT); //PB switch power low for Low power
  digitalWrite(Trigger_switch, LOW);
  pinMode(Step_EN, OUTPUT);
  digitalWrite(Step_EN, HIGH); //3.3v on
  pinMode(ADC_0, OUTPUT);
  digitalWrite(ADC_0, HIGH);
  digitalWrite(Step_EN, HIGH); //3V on
  pinMode(LED, OUTPUT);//groene led
  digitalWrite(LED, HIGH);//groene led
  pinMode(Buzzer, OUTPUT);//buzzer
  digitalWrite(Buzzer, LOW);//buzzer
  pinMode(Esp_power, OUTPUT);//PB Bridge
  digitalWrite(Esp_power, LOW);//PB Bridge
  pinMode(Esp_RST, OUTPUT);//esp reset
  digitalWrite(Esp_RST, HIGH);//esp reset
  delay(5000);
  Serial1.begin(38400);
  Serial.begin(38400);
  //Serial.println("AT+CIOBAUD=9600");
  Serial.println("Start");
  Serial1.println("AT");
  delay(5000);
  if(Serial1.find("OK")){
    connectWiFi();
  }
}

void loop(){

  float tempC = thermocouple.readCelsius();
  char buffer[10];
  String tempF = dtostrf(tempC, 4, 1, buffer);
  tempF.trim();
  updateTemp(tempF);
  delay(6000);
}

void updateTemp(String tenmpF){
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += IP;
  cmd += "\",80";
  Serial1.println(cmd);
  delay(2000);
  if(Serial1.find("Error")){
    return;
  }
  cmd = GET;
  cmd += tenmpF;
  cmd += "\r\n";
  Serial1.print("AT+CIPSEND=");
  Serial1.println(cmd.length());
  if(Serial1.find(">")){
    Serial1.print(cmd);
    Serial.println(cmd);
  }else{
    Serial1.println("AT+CIPCLOSE");
  }
}

 
boolean connectWiFi(){
  Serial1.println("AT+CWMODE=1");
  delay(2000);
  String cmd="AT+CWJAP=\"";
  cmd+=SSID;
  cmd+="\",\"";
  cmd+=PASS;
  cmd+="\"";
  Serial1.println(cmd);
  delay(5000);
  if(Serial1.find("OK")){
    Serial.println("Connected to WiFi");
    tone(Buzzer,2000,100);
    return true;
  }else{
    return false;
  }
}
