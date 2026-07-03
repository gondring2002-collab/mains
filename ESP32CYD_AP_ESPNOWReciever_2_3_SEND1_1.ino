#include <WiFi.h>
#include <esp_now.h>
#include <ESPAsyncWebServer.h>
#include <TFT_eSPI.h>

/* ================= TFT ================= */

TFT_eSPI tft = TFT_eSPI();

/* ================= WIFI ================= */

const char* ssid = "ANTOPHOMETRI-TINGGI";
const char* password = "123456789";

AsyncWebServer server(80);

/* ================= SENSOR ================= */

float beratBadan = 0;
float tinggiBadan = 0;
long angka ;
long angka1;

bool kalAck = false;
/* ================= STRUCT ================= */

uint8_t macVL53[] = {0xC0, 0x5D, 0x89, 0xB0, 0x36, 0x18};//C0:5D:89:B0:36:18
uint8_t macHX711[] ={0x48, 0xE7, 0x29, 0x94, 0x0A, 0x50};//48:E7:29:94:0A:50
esp_now_peer_info_t peerInfo;
uint8_t rotationTFT = 0;

typedef struct __attribute__((packed))
{
  uint8_t nodeID;
  uint32_t packetID;
  float value;
  uint8_t rotationID;
} sensorPacket;

sensorPacket incomingData;
sensorPacket sendData;


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
  if(var == "TEMPERATURE") return String(tinggiBadan,2);
  if(var == "HUMIDITY") return String(beratBadan,2);
  return String();
}

/* ================= ESP NOW RECEIVE ================= */

void onReceive(const esp_now_recv_info_t *info,
               const uint8_t *data,
               int len)
{
  if(len != sizeof(sensorPacket)) return;

  memcpy(&incomingData, data, sizeof(sensorPacket));

  if(incomingData.nodeID == 2)
  {
    angka1 = random(100);
    beratBadan = incomingData.value;// +angka1;
    //Serial.print("BB : ");
    //Serial.println(beratBadan);
  }

  else if(incomingData.nodeID == 1)
  {
    //angka = random(100);
    tinggiBadan = incomingData.value;//+angka;
    rotationTFT = incomingData.rotationID;
    //Serial.print("TB : ");
    //Serial.println(tinggiBadan);
  }
  if (incomingData.nodeID == 198)
  {
    kalAck = true;

    Serial.print("KAL OK : ");
    Serial.println(incomingData.value);
  }
  if (incomingData.nodeID == 199)
{
    Serial.print("HX711 OK : ");
    Serial.println(incomingData.value);
}
}

/* ================= TFT ================= */

void updateLCD()
{
  if(rotationTFT == 0)
  {
      tft.setRotation(2);   // Tidur
  }
  else
  {
      tft.setRotation(1);   // Berdiri
  }

  tft.fillScreen(TFT_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN);
  tft.drawString("ESP32 GATEWAY",20,10);

  tft.setTextColor(TFT_YELLOW);

  tft.drawString("Berat (BB):",20,60);
  tft.drawFloat(beratBadan,2,150,60);
  tft.drawString("kg",220,60);

  tft.drawString("Tinggi (TB):",20,100);
  tft.drawFloat(tinggiBadan,2,165,100);
    tft.drawString("mm",235,100);

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
  
// ================== Peer VL53 ==================
memcpy(peerInfo.peer_addr, macVL53, 6);
peerInfo.channel = 1;
peerInfo.encrypt = false;

if (esp_now_add_peer(&peerInfo) == ESP_OK)
{
  Serial.println("Peer VL53 OK");
}
else
{
  Serial.println("Peer VL53 GAGAL");
}

// ================== Peer HX711 ==================
memcpy(peerInfo.peer_addr, macHX711, 6);

if (esp_now_add_peer(&peerInfo) == ESP_OK)
{
  Serial.println("Peer HX711 OK");
}
else
{
  Serial.println("Peer HX711 GAGAL");
}
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

  if (millis() - t > 500)
  {
    t = millis();
    updateLCD();
  }

  if (Serial.available())
{
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.startsWith("VL53="))
  {
    float kal = cmd.substring(5).toFloat();

    sendData.nodeID = 98;
    sendData.packetID = 0;
    sendData.value = kal;
    sendData.rotationID = 0;

    esp_err_t result = esp_now_send(macVL53, (uint8_t *)&sendData, sizeof(sendData));

    Serial.print("Kirim offset VL53 : ");
    Serial.println(kal);

    if (result == ESP_OK)
      Serial.println("Kirim OK");
    else
      Serial.println("Kirim GAGAL");
   }
   if (cmd.startsWith("HX711="))
  {
    float kal = cmd.substring(6).toFloat();

    sendData.nodeID = 99;
    sendData.packetID = 0;
    sendData.value = kal;
    sendData.rotationID = 0;

    esp_err_t result = esp_now_send(macHX711,
                                    (uint8_t *)&sendData,
                                    sizeof(sendData));

    Serial.print("Kirim kalibrasi HX711 : ");
    Serial.println(kal);

    if (result == ESP_OK)
        Serial.println("Kirim HX711 OK");
    else
        Serial.println("Kirim HX711 GAGAL");
  }
 }
}