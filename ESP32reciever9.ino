#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <U8g2lib.h>

const char* ssid = "ESP8266-Access-Point";
const char* password = "123456789";

AsyncWebServer server(80);

// OLED SH1106 / SSD1306
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

float t = 0;
float h = 0;

//int t = 0;
//int h = 0;

unsigned long previousMillis = 0;
const long interval = 500;
//const long interval = 10000;


// ================= HTML =================

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
</head>

<body>
<h2>ESP32 Sensor Server</h2>

<p>
Temperature :
<span id="temperature">%TEMPERATURE%</span>
</p>

<p>
Humidity :
<span id="humidity">%HUMIDITY%</span>
</p>

</body>

<script>

setInterval(function(){

fetch("/temperature")
.then(response => response.text())
.then(data => document.getElementById("temperature").innerHTML = data);

//},10000);
},500);

setInterval(function(){

fetch("/humidity")
.then(response => response.text())
.then(data => document.getElementById("humidity").innerHTML = data);

//},10000);
},500);

</script>

</html>)rawliteral";


// ================= HTML Processor =================

String processor(const String& var){

  if(var == "TEMPERATURE")
    return String(t);

  if(var == "HUMIDITY")
    return String(h);

  return String();
}


// ================= OLED =================

void updateOLED()
{
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_ncenB08_tr);

  u8g2.drawStr(0,12,"ESP32 SENSOR");

  String line1 = "Temp : " + String(t,1) + " C";
  String line2 = "Hum  : " + String(h,1) + " %";

  u8g2.drawStr(0,32,line1.c_str());
  u8g2.drawStr(0,48,line2.c_str());

  u8g2.sendBuffer();
}


// ================= SETUP =================

void setup(){

  Serial.begin(115200);

  // OLED start
  u8g2.begin();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(10,30,"STARTING...");
  u8g2.sendBuffer();


  // Start Access Point
  WiFi.softAP(ssid, password);

  Serial.println("Access Point Started");

  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());


  // Web routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200,"text/html",index_html, processor);
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200,"text/plain",String(t).c_str());
    //String temp = String(t,1);
    //temp.replace(".",",");
    //request->send(200,"text/plain",temp);
  });

  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200,"text/plain",String(h).c_str());
    //String hum = String(h,1);
    //hum.replace(".",",");
    //request->send(200,"text/plain",hum);
  });

  server.begin();

  updateOLED();
}


// ================= LOOP =================

void loop(){

  unsigned long currentMillis = millis();

  if(currentMillis - previousMillis >= interval){

    previousMillis = currentMillis;

    // RANDOM SENSOR DATA
    t = random(200,350) / 10.0;
    //t = random(20,35);
    h = random(400,800) / 10.0;
    //h = random(40,80);

    Serial.print("Temperature : ");
    Serial.println(t);

    Serial.print("Humidity : ");
    Serial.println(h);

    updateOLED();
  }
}
