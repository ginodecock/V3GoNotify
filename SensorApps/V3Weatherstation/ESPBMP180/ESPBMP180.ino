#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>


/////////////////////////
// Network Definitions //
/////////////////////////
const IPAddress AP_IP(192, 168, 4, 1);
const char* AP_SSID = "GoNotifyWeather";
boolean SETUP_MODE;
String SSID_LIST;
DNSServer DNS_SERVER;
ESP8266WebServer WEB_SERVER(80);
String wifiPassword;
String wifiSsid;

/////////////////////////
// Device Definitions //
/////////////////////////
String DEVICE_TITLE = "GoNotifyWeather";

///////////////////////
// Wunderground Definitions //
///////////////////////
const char* Wunderground_URL= "rtupdate.wunderground.com";
String ID= "";
String PASSWORD = "";

///////////////////////
// Trigger Definitions //
///////////////////////
int L_TEMP= 0;
int H_TEMP= 10;

String str;
String recValue_1 = "1";
String recValue_2 = "2";
String recValue_3 = "3";
void initHardware()
{
  // Serial and EEPROM
  Serial.begin(38400);
  EEPROM.begin(512);
  delay(10);
}
void setup() {
  initHardware();
}
//////////////////////
// Button Functions //
//////////////////////
void triggerButtonEvent(String eventName, String key)
{
  // Define the WiFi Client
  WiFiClientSecure client;

  if (!client.connect(Wunderground_URL, 443)) {
    Serial.println("<No_Http_Connection");
    Serial.println("No connection with: " + String(Wunderground_URL));
    return;
  }
  String value_1 = "";
  String value_2 = "";
  value_1 = recValue_1;//String(BUTTON_COUNTER -1);
  value_2 = recValue_2;//ipStr;
  
  // We now create a URI for the request
  String url = "/weatherstation/updateweatherstation.php?ID=" + String(ID) + "&PASSWORD=" + String(PASSWORD) + "&action=updateraw&dateutc=now&" + value_1 + "&" + value_2;
  Serial.println(url);
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + Wunderground_URL + "\r\n" +
               "Connection: close\r\n\r\n");
  // Read all the lines of the reply from server and print them to Serial
  Serial.println("Request sent - waiting for reply...");

  delay(1000);
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  Serial.println();
  Serial.println("closing connection");
}

/////////////////////////////
// AP Setup Mode Functions //
/////////////////////////////

// Load Saved Configuration from EEPROM
boolean loadSavedConfig() {
  Serial.println("Reading Saved Config....");
  String ssid = "";
  String password = "";
  String lID ="";
  String lPASSWORD ="";
  String lL_TEMP ="";
  String lH_TEMP =""; 
  if (EEPROM.read(0) != 0) {
    for (int i = 0; i < 32; ++i) {
      if (EEPROM.read(i) == 0) {break;};
      ssid += char(EEPROM.read(i));
    }
    Serial.print("SSID: ");
    Serial.println(ssid);
    wifiSsid = ssid;
    for (int i = 32; i < 64; ++i) {
      if (EEPROM.read(i) == 0) {break;};
      password += char(EEPROM.read(i));
    }
    Serial.print("Password: ");
    Serial.println(password);
    wifiPassword = password;

    for (int i = 64; i < 96; ++i) {
      if (EEPROM.read(i) == 0) {break;};
      lID += char(EEPROM.read(i));
    }
    Serial.print("ID: ");
    lID.trim();
    ID = lID.substring(0,24);
    Serial.println(ID);

    for (int i = 96; i < 128; ++i) {
      if (EEPROM.read(i) == 0) {break;};
      lPASSWORD += char(EEPROM.read(i));
    }
    Serial.print("PASSWORD: ");
    Serial.println(lPASSWORD);
    PASSWORD = lPASSWORD;
    for (int i = 128; i < 160; ++i) {
      if (EEPROM.read(i) == 0) {break;};
      lL_TEMP += char(EEPROM.read(i));
    }
    L_TEMP = lL_TEMP.toInt();
    Serial.print("L_TEMP: ");
    Serial.println(L_TEMP);

    for (int i = 160; i < 192; ++i) {
      if (EEPROM.read(i) == 0) {break;};
      lH_TEMP += char(EEPROM.read(i));
    }
    H_TEMP = lH_TEMP.toInt();
    Serial.print("H_TEMP: ");
    Serial.println(H_TEMP);
    WiFi.scanNetworks();
    WiFi.begin(ssid.c_str(), password.c_str());
    return true;
  }
  else {
    Serial.println("Saved Configuration not found.");
    return false;
  }
}

// Boolean function to check for a WiFi Connection
boolean checkWiFiConnection() {
  int count = 0;
  Serial.print("Waiting to connect to the specified WiFi network");
  while ( count < 270 ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("<OK_Connection");
      Serial.println("Connected!");
      
      return (true);
    }
    delay(1000);
    Serial.print(".");
    count++;
  }
  Serial.println("Timed out.");
  return false;
}
String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}
String makePage(String title, String contents) {
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<style>";
  // Simple Reset CSS
  s += "*,*:before,*:after{-webkit-box-sizing:border-box;-moz-box-sizing:border-box;box-sizing:border-box}html{font-size:100%;-ms-text-size-adjust:100%;-webkit-text-size-adjust:100%}html,button,input,select,textarea{font-family:sans-serif}article,aside,details,figcaption,figure,footer,header,hgroup,main,nav,section,summary{display:block}body,form,fieldset,legend,input,select,textarea,button{margin:0}audio,canvas,progress,video{display:inline-block;vertical-align:baseline}audio:not([controls]){display:none;height:0}[hidden],template{display:none}img{border:0}svg:not(:root){overflow:hidden}body{font-family:sans-serif;font-size:16px;font-size:1rem;line-height:22px;line-height:1.375rem;color:#585858;font-weight:400;background:#fff}p{margin:0 0 1em 0}a{color:#cd5c5c;background:transparent;text-decoration:underline}a:active,a:hover{outline:0;text-decoration:none}strong{font-weight:700}em{font-style:italic}";
  // Basic CSS Styles
  s += "body{font-family:sans-serif;font-size:16px;font-size:1rem;line-height:22px;line-height:1.375rem;color:#585858;font-weight:400;background:#fff}p{margin:0 0 1em 0}a{color:#cd5c5c;background:transparent;text-decoration:underline}a:active,a:hover{outline:0;text-decoration:none}strong{font-weight:700}em{font-style:italic}h1{font-size:32px;font-size:2rem;line-height:38px;line-height:2.375rem;margin-top:0.7em;margin-bottom:0.5em;color:#343434;font-weight:400}fieldset,legend{border:0;margin:0;padding:0}legend{font-size:18px;font-size:1.125rem;line-height:24px;line-height:1.5rem;font-weight:700}label,button,input,optgroup,select,textarea{color:inherit;font:inherit;margin:0}input{line-height:normal}.input{width:100%}input[type='text'],input[type='email'],input[type='tel'],input[type='date']{height:36px;padding:0 0.4em}input[type='checkbox'],input[type='radio']{box-sizing:border-box;padding:0}";
  // Custom CSS
  s += "header{width:100%;background-color: #01DF01;top: 0;min-height:60px;margin-bottom:21px;font-size:15px;color: #fff}.content-body{padding:0 1em 0 1em}header p{font-size: 1.25rem;float: left;position: relative;z-index: 1000;line-height: normal; margin: 15px 0 0 10px}";
  s += "</style>";
  s += "<title>";
  s += title;
  s += "</title></head><body>";
  s += "<header><p>" + DEVICE_TITLE + "</p></header>";
  s += "<div class=\"content-body\">";
  s += contents;
  s += "</div>";
  s += "</body></html>";
  return s;
}
// Start the web server and build out pages
void startWebServer() {
  if (SETUP_MODE) {
    Serial.print("Starting Web Server at IP address: ");
    Serial.println(WiFi.softAPIP());
    // Settings Page
    WEB_SERVER.on("/settings", []() {
        String webwifiPassword;
        String webwifiSsid;
        String webID;
        String webPASSWORD;
        if (EEPROM.read(0) != 0) {
        for (int i = 0; i < 32; ++i) {
          if (EEPROM.read(i) == 0) {break;};
          webwifiSsid += char(EEPROM.read(i));
        }
        for (int i = 32; i < 64; ++i) {
          if (EEPROM.read(i) == 0) {break;};
          webwifiPassword += char(EEPROM.read(i));
        }
        for (int i = 64; i < 96; ++i) {
          if (EEPROM.read(i) == 0) {break;};
          webID += char(EEPROM.read(i));
        }
        for (int i = 96; i < 128; ++i) {
          if (EEPROM.read(i) == 0) {break;};
          webPASSWORD += char(EEPROM.read(i));
        }
      }
      String s = "<h2>Settings</h2><p>Please select the SSID of the network you wish to connect to and then enter the password and submit.</p>";
      s += "<form method=\"get\" action=\"setap\"><label>SSID: </label><select name=\"ssid\">";
      s += SSID_LIST;
      s += "</select><br><br>Password: ";
      s += "<input type=\"text\" name=\"pass\" value=\""; 
      s += webwifiPassword;
      s += "\"length=32 maxlength=32 ><br>";
      s += "<br><br> <h3>Wunderground Settings</h3>";
      s += "Station ID: <br>";
      s += "<input type=\"text\" name=\"ID\" placeholder=\"" + String(webID) + "\" value=\"" + String(webID) + "\" size=32 maxlength=32 ><br>";
      s += "PASSWORD: <br>";
      s += "<input type=\"text\" name=\"PASSWORD\" placeholder=\"" + String(webPASSWORD) + "\" value=\"" + String(webPASSWORD) + "\" size=32 maxlength=32 ><br>";
      s += "<br><br> <h3>Trigger Settings</h3>";
      s += "Low temperature: <br>";
      s += "<input type=\"number\" name=\"LTEMP\" placeholder=\"" + String(L_TEMP) + "\" value=\"" + String(L_TEMP) + "\" size=5 maxlength=5 >Celcius<br>";
      s += "High temperature: <br>";
      s += "<input type=\"number\" name=\"HTEMP\" placeholder=\"" + String(H_TEMP) + "\" value=\"" + String(H_TEMP) + "\" size=5 maxlength=5 >Celcius<br>";
      s += "<br><input type=\"submit\"></form><br>";
      WEB_SERVER.send(200, "text/html", makePage("Wi-Fi Settings", s));
    });
    // setap Form Post
    WEB_SERVER.on("/setap", []() {
      for (int i = 0; i < 96; ++i) {
        EEPROM.write(i, 0);
      }
      String ssid = urlDecode(WEB_SERVER.arg("ssid"));
      Serial.print("SSID: ");
      Serial.println(ssid);
      String pass = urlDecode(WEB_SERVER.arg("pass"));
      Serial.print("Password: ");
      Serial.println(pass);
      String ID = urlDecode(WEB_SERVER.arg("ID"));
      Serial.print("ID: ");
      Serial.println(ID);
      String PASSWORD = urlDecode(WEB_SERVER.arg("PASSWORD"));
      Serial.print("PASSWORD: ");
      Serial.println(PASSWORD);
      String L_TEMP = urlDecode(WEB_SERVER.arg("LTEMP"));
      Serial.print("L_TEMP: ");
      Serial.println(L_TEMP);
      String H_TEMP = urlDecode(WEB_SERVER.arg("HTEMP"));
      Serial.print("H_TEMP: ");
      Serial.println(H_TEMP);
      Serial.println("Writing SSID to EEPROM...");
      for (int i = 0; i < ssid.length(); ++i) {
        EEPROM.write(i, ssid[i]);
      }
      Serial.println("Writing Password to EEPROM...");
      for (int i = 0; i < pass.length(); ++i) {
        EEPROM.write(32 + i, pass[i]);
      }
      Serial.println("Writing ID to EEPROM...");
      for (int i = 0; i < ID.length()+1; ++i) {
        EEPROM.write(64 + i, ID[i]);
      }
      Serial.println("Writing PASSWORD to EEPROM...");
      for (int i = 0; i < PASSWORD.length()+1; ++i) {
        EEPROM.write(96 + i, PASSWORD[i]);
      }
      Serial.println("Writing LTEMP to EEPROM...");
      for (int i = 0; i < L_TEMP.length(); ++i) {
        EEPROM.write(128 + i, L_TEMP[i]);
      }
      Serial.println("Writing HTEMP to EEPROM...");
      for (int i = 0; i < H_TEMP.length(); ++i) {
        EEPROM.write(160 + i, H_TEMP[i]);
      }
      EEPROM.commit();
      Serial.println("Write EEPROM done!");
      String s = "<h1>Setup complete.</h1><p>GoNotifyWeather will be connected automatically to \"";
      s += ssid;
      s += "\" after the restart.";
      WEB_SERVER.send(200, "text/html", makePage("Wi-Fi Settings", s));
      ESP.restart();
    });
    // Show the configuration page if no path is specified
    WEB_SERVER.onNotFound([]() {
      String s = "<h1>Configuration Mode</h1><p><a href=\"/settings\">Settings</a></p>";
      WEB_SERVER.send(200, "text/html", makePage("Access Point mode", s));
    });
  }
  else {
    Serial.print("Starting Web Server at ");
    Serial.println(WiFi.localIP());
    WEB_SERVER.on("/", []() {
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      String s = "<h1>GoNotifyWeather Status</h1>";
      s += "<h3>Network Details</h3>";
      s += "<p>Connected to: " + String(WiFi.SSID()) + "</p>";
       s += "<p>IP Address: " + ipStr + "</p>";
      s += "<h3>Wunderground details</h3>";
      s += "<p>PASSWORD: " + String(PASSWORD) + "</p>";
      s += "<p>ID: " + String(ID) + "</p>";
      s += "<h3>Options</h3>";
      s += "<p><a href=\"/reset\">Clear Saved Wi-Fi Settings</a></p>";
      WEB_SERVER.send(200, "text/html", makePage("Station mode", s));
    });
    WEB_SERVER.on("/reset", []() {
      for (int i = 0; i < 192; ++i) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
      String s = "<h1>Settings are reset.</h1><p>Please reset device.</p>";
      WEB_SERVER.send(200, "text/html", makePage("Reset Wi-Fi Settings", s));
    });
  }
  WEB_SERVER.begin();
}


// Build the SSID list and setup a software access point for setup mode
void setupMode() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(100);
  Serial.println("");
  for (int i = 0; i < n; ++i) {
    SSID_LIST += "<option value=\"";
    SSID_LIST += WiFi.SSID(i);
    SSID_LIST += "\"";
    if (String(WiFi.SSID(i)) == wifiSsid){
      SSID_LIST += " selected";
    }
    SSID_LIST += ">";
    SSID_LIST += WiFi.SSID(i);
    SSID_LIST += "</option>";
  }
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID);
  DNS_SERVER.start(53, "*", AP_IP);
  startWebServer();
  Serial.print("Starting Access Point at \"");
  Serial.print(AP_SSID);
  Serial.println("\"");
}




/////////////////////////
// Debugging Functions //
/////////////////////////

void wipeEEPROM()
{
  EEPROM.begin(512);
  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < 512; i++)
    EEPROM.write(i, 0);

  EEPROM.end();
}
void loop() {

  // Handle WiFi Setup and Webserver for reset page
  if (SETUP_MODE) {
    DNS_SERVER.processNextRequest();
  }
  WEB_SERVER.handleClient();

   while(Serial.available()) {
      str = Serial.readStringUntil('\n');
  }
  
  if (str != "") {
    //Serial.println(str);
    str.trim();
    if (str == ">Push"){
      Serial.println("<");
      // Try and restore saved settings
      if (loadSavedConfig()) {
        if (checkWiFiConnection()) {
        SETUP_MODE = false;
        //startWebServer();
        // Turn the status led Green when the WiFi has been connected
        //digitalWrite(LED_GREEN, HIGH);
        Serial.println("<WiFi_Connected");delay(20);
      } else{
        Serial.println("<No_WiFi_Connection");delay(20);
      }
    } else {
      Serial.println("<No_Config");delay(20);
    }
      triggerButtonEvent(PASSWORD,ID);
      delay(1000);
      Serial.println("<Done_Sending");delay(100);
    }
    if (str == ">ConfigWiFi"){
      Serial.println("<");
      String lL_TEMP ="";
      String lH_TEMP =""; 
      if (EEPROM.read(0) != 0) {
        for (int i = 0; i < 32; ++i) {
          if (EEPROM.read(i) == 0) {break;};
          wifiSsid += char(EEPROM.read(i));
        }
        Serial.print("SSID: ");
        Serial.println(wifiSsid);
        
        for (int i = 32; i < 64; ++i) {
          if (EEPROM.read(i) == 0) {break;};
          wifiPassword += char(EEPROM.read(i));
        }
        Serial.print("Password: ");
        Serial.println(wifiPassword);

        for (int i = 64; i < 96; ++i) {
          if (EEPROM.read(i) == 0) {break;};
          ID += char(EEPROM.read(i));
        }
        Serial.print("ID: ");
        Serial.println(ID);

        for (int i = 96; i < 128; ++i) {
          if (EEPROM.read(i) == 0) {break;};
          PASSWORD += char(EEPROM.read(i));
        }
        Serial.print("PASSWORD: ");
        Serial.println(PASSWORD);

        for (int i = 128; i < 160; ++i) {
          if (EEPROM.read(i) == 0) {break;};
          lL_TEMP += char(EEPROM.read(i));
        }
        L_TEMP = lL_TEMP.toInt();
        Serial.print("<L_TEMP=");
        Serial.println(L_TEMP);
        for (int i = 160; i < 192; ++i) {
          if (EEPROM.read(i) == 0) {break;};
          lH_TEMP += char(EEPROM.read(i));
        }
        H_TEMP = lH_TEMP.toInt();
        Serial.print("<H_TEMP=");
        Serial.println(H_TEMP);
      } else {
        Serial.println("<NoConfig");
        delay(100);
      }
      SETUP_MODE = true;
      setupMode();delay(5);
    }
    if (str.startsWith(">value1=")){
      recValue_1 = str.substring(8);
      Serial.println("<");
    }
    if (str.startsWith(">value2=")){
      recValue_2 = str.substring(8);
      Serial.println("<");
    }
    if (str.startsWith(">value3=")){
      recValue_3 = str.substring(8);
      Serial.println("<");
    }
    str = "";
  }
}
