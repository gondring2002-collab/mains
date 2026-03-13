#include <WiFi.h>
#include <esp_now.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <U8g2lib.h>

/* ================= OLED ================= */

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

/* ================= WIFI AP ================= */

const char* ssid = "ESP32-Gateway";
const char* password = "12345678";

AsyncWebServer server(80);

/* ================= NODE MAC ================= */

uint8_t node1MAC[] = {0x48,0xE7,0x29,0x94,0x0A,0x50}; // Berat Badan
uint8_t node2MAC[] = {0xC0,0x5D,0x89,0xB1,0x35,0xE0}; // Tinggi Badan

#define NODE_TIMEOUT 5000

/* ================= SENSOR VALUE ================= */

float beratBadan = 0;
float tinggiBadan = 0;

/* ================= PACKET TRACKING ================= */

uint32_t lastPacketNode1 = 0;
uint32_t lastPacketNode2 = 0;

unsigned long lastSeenNode1 = 0;
unsigned long lastSeenNode2 = 0;

/* ================= STRUCT DATA ================= */

typedef struct _attribute_((packed))
{
  uint8_t nodeID;
  uint32_t packetID;
  float value;
} sensorPacket;

sensorPacket incomingData;

/* ================= NODE STATUS ================= */

bool node1Online()
{
  return (millis() - lastSeenNode1) < NODE_TIMEOUT;
}

bool node2Online()
{
  return (millis() - lastSeenNode2) < NODE_TIMEOUT;
}

/* ================= OLED DISPLAY ================= */

void updateOLED()
{

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  char line[32];

  sprintf(line,"BB : %.2f kg",beratBadan);
  u8g2.drawStr(0,20,line);

  sprintf(line,"TB : %.2f cm",tinggiBadan);
  u8g2.drawStr(0,40,line);

  sprintf(line,"N1:%s N2:%s",
          node1Online() ? "OK":"OFF",
          node2Online() ? "OK":"OFF");

  u8g2.drawStr(0,60,line);

  u8g2.sendBuffer();
}

/* ================= HTML PAGE ================= */

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
.then(response => response.text())
.then(data => document.getElementById("temperature").innerHTML = data);

},500);

setInterval(function(){

fetch("/humidity")
.then(response => response.text())
.then(data => document.getElementById("humidity").innerHTML = data);

},500);

</script>

</body>
</html>)rawliteral";

/* ================= HTML PROCESSOR ================= */

String processor(const String& var)
{
  if(var == "TEMPERATURE")
    return String(beratBadan,2);

  if(var == "HUMIDITY")
    return String(tinggiBadan,2);

  return String();
}

/* ================= ESP NOW RECEIVE ================= */

void onReceive(const esp_now_recv_info_t *info,
               const uint8_t *data,
               int len)
{

  if(len != sizeof(sensorPacket))
  {
    Serial.println("Invalid packet size");
    return;
  }

  memcpy(&incomingData, data, sizeof(sensorPacket));

  const uint8_t *mac = info->src_addr;

  /* NODE 1 : BERAT BADAN */

  if (memcmp(mac,node1MAC,6)==0 && incomingData.nodeID==1)
  {

    if(incomingData.packetID > lastPacketNode1)
    {

      lastPacketNode1 = incomingData.packetID;

      beratBadan = incomingData.value;

      lastSeenNode1 = millis();

      Serial.print("BB : ");
      Serial.println(beratBadan,2);
    }
  }

  /* NODE 2 : TINGGI BADAN */

  else if (memcmp(mac,node2MAC,6)==0 && incomingData.nodeID==2)
  {

    if(incomingData.packetID > lastPacketNode2)
    {

      lastPacketNode2 = incomingData.packetID;

      tinggiBadan = incomingData.value;

      lastSeenNode2 = millis();

      Serial.print("TB : ");
      Serial.println(tinggiBadan,2);
    }
  }
}

/* ================= SETUP ================= */

void setup()
{

  Serial.begin(115200);

  Wire.begin();

  /* OLED START */

  u8g2.begin();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(10,30,"Gateway Start");
  u8g2.sendBuffer();

  delay(1500);

  /* WIFI AP */

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid,password);

  Serial.print("Gateway IP : ");
  Serial.println(WiFi.softAPIP());

  /* ESP NOW */

  if (esp_now_init()!=ESP_OK)
  {
    Serial.println("ESP NOW INIT ERROR");
    return;
  }

  esp_now_register_recv_cb(onReceive);

  /* WEB SERVER */

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send_P(200,"text/html",index_html,processor);
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(200,"text/plain",String(beratBadan,2));
  });

  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(200,"text/plain",String(tinggiBadan,2));
  });

  server.begin();

}

/* ================= LOOP ================= */

void loop()
{

  static unsigned long oledTimer = 0;
  static unsigned long serialTimer = 0;

  if(millis()-oledTimer > 500)
  {
    oledTimer = millis();
    updateOLED();
  }

  if(millis()-serialTimer > 2000)
  {

    serialTimer = millis();

    Serial.print("Node1:");
    Serial.print(node1Online());

    Serial.print(" | Node2:");
    Serial.println(node2Online());

  }

}
