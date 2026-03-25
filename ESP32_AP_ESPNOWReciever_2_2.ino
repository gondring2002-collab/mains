#include <WiFi.h>
#include <esp_now.h>
#include <ESPAsyncWebServer.h>
#include <TFT_eSPI.h>

/* ================= TFT ================= */

TFT_eSPI tft = TFT_eSPI();

/* ================= WIFI ================= */

const char* ssid = "ESP32-GATEWAY";
const char* password = "12345678";

AsyncWebServer server(80);

/* ================= SENSOR ================= */

float beratBadan = 0;
float tinggiBadan = 0;

/* ================= STRUCT ================= */

typedef struct __attribute__((packed))
{
  uint8_t nodeID;
  uint32_t packetID;
  float value;
} sensorPacket;

sensorPacket incomingData;

/* ================= HTML ================= */

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Gateway</title>
</head>
<body>

<h2>ESP32 Sensor Server</h2>

<p>
Berat Badan :
<span id="temperature">%TEMPERATURE%</span> kg
</p>

<p>
Tinggi Badan :
<span id="humidity">%HUMIDITY%</span> cm
</p>

<script>

setInterval(function(){
fetch("/temperature")
.then(r => r.text())
.then(d => document.getElementById("temperature").innerHTML = d);
},500);

setInterval(function(){
fetch("/humidity")
.then(r => r.text())
.then(d => document.getElementById("humidity").innerHTML = d);
},500);

</script>

</body>
</html>
)rawliteral";

/* ================= HTML PROCESS ================= */

String processor(const String& var)
{
  if(var == "TEMPERATURE") return String(beratBadan,2);
  if(var == "HUMIDITY") return String(tinggiBadan,2);
  return String();
}

/* ================= ESP NOW RECEIVE ================= */

void onReceive(const esp_now_recv_info_t *info,
               const uint8_t *data,
               int len)
{
  if(len != sizeof(sensorPacket)) return;

  memcpy(&incomingData, data, sizeof(sensorPacket));

  if(incomingData.nodeID == 1)
  {
    beratBadan = incomingData.value;
    Serial.print("BB : ");
    Serial.println(beratBadan);
  }

  else if(incomingData.nodeID == 2)
  {
    tinggiBadan = incomingData.value;
    Serial.print("TB : ");
    Serial.println(tinggiBadan);
  }
}

/* ================= TFT ================= */

void updateLCD()
{
  tft.fillScreen(TFT_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN);
  tft.drawString("ESP32 GATEWAY",20,10);

  tft.setTextColor(TFT_YELLOW);

  tft.drawString("Berat :",20,60);
  tft.drawFloat(beratBadan,2,120,60);

  tft.drawString("Tinggi :",20,100);
  tft.drawFloat(tinggiBadan,2,120,100);

  tft.setTextColor(TFT_CYAN);
  tft.drawString("IP:",20,200);
  tft.drawString(WiFi.softAPIP().toString(),80,200);
}

/* ================= SETUP ================= */

void setup()
{
  Serial.begin(115200);

  /* TFT */
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Start...",100,100);

  /* WIFI AP + STA */
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password, 1);

  WiFi.STA.begin();   // penting untuk ESP-NOW
  delay(100);

  Serial.println("AP READY");
  Serial.println(WiFi.softAPIP());

  /* ESP NOW */
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP NOW ERROR");
    return;
  }

  esp_now_register_recv_cb(onReceive);

  /* WEB SERVER */

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(beratBadan,2));
  });

  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(tinggiBadan,2));
  });

  server.begin();
}

/* ================= LOOP ================= */

void loop()
{
  static unsigned long t = 0;

  if(millis() - t > 500)
  {
    t = millis();
    updateLCD();
  }
}