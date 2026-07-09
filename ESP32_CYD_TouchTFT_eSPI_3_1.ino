/*******************************************************
 * GUI-06
 * CYD ESP32-2432S028
 * TFT_eSPI + XPT2046_Touchscreen
 *******************************************************/
#include <WiFi.h>
#include <esp_now.h>
#include <ESPAsyncWebServer.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "logoBRIN.h"
#include "logoLPDP.h"
#include "logoWIFI.h"
#include "logoBLUETOOTH.h"
//====================================================
// TFT
//====================================================

TFT_eSPI tft = TFT_eSPI();
/* ================= WIFI ================= */

const char* ssid = "ANTOPHOMETRI-TINGGI";
const char* password = "123456789";

AsyncWebServer server(80);

/* ================= SENSOR ================= */

float beratBadan = 10;
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

uint8_t currentRotation = 255;

int lcdW = tft.width();
int lcdH = tft.height();

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
   // angka1 = random(100);
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
//====================================================
// TOUCH
//====================================================

#define XPT2046_CS   33
#define XPT2046_IRQ  36

SPIClass touchSPI(HSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

//====================================================
// KALIBRASI TOUCH
//====================================================

#define TOUCH_X_MIN 445
#define TOUCH_X_MAX 3491

#define TOUCH_Y_MIN 713
#define TOUCH_Y_MAX 3493

//====================================================
// WARNA
//====================================================

#define BG_COLOR       TFT_BLACK
#define HEADER_COLOR   TFT_WHITE

#define BERAT_COLOR    TFT_YELLOW
#define TINGGI_COLOR   TFT_GREEN

#define BTN1_COLOR     TFT_BLUE
#define BTN2_COLOR     TFT_GREEN

//====================================================
// POSISI TOMBOL
//====================================================

int BTN1_X;
int BTN1_Y;
int BTN1_W;
int BTN1_H;

int BTN2_X;
int BTN2_Y;
int BTN2_W;
int BTN2_H;
//====================================================
// DATA DEMO
//====================================================

float berat = 68.25;
float tinggi = 172.4;
float lastBerat = -999;
float lastTinggi = -999;
//====================================================
// TOUCH
//====================================================

uint16_t touchX = 0;
uint16_t touchY = 0;

//====================================================
// MAPPING
//====================================================



const unsigned char logo16[] PROGMEM =
{
0x03,0xC0,
0x0F,0xF0,
0x1F,0xF8,
0x3F,0xFC,
0x7F,0xFE,
0xFF,0xFF,
0xFF,0xFF,
0x7F,0xFE,
0x3F,0xFC,
0x1F,0xF8,
0x0F,0xF0,
0x07,0xE0,
0x03,0xC0,
0x01,0x80,
0x00,0x00,
0x00,0x00
};

int getTouchX(int rawX)
{
    return map(rawX,
               TOUCH_X_MIN,
               TOUCH_X_MAX,
               0,
               319);
}

int getTouchY(int rawY)
{
    return map(rawY,
               TOUCH_Y_MIN,
               TOUCH_Y_MAX,
               0,
               239);
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
//====================================================
// BUTTON
//====================================================

void drawButton(int x,int y,int w,int h,uint16_t color,const char *text)
{
    tft.fillRoundRect(x,y,w,h,8,color);

    tft.drawRoundRect(x,y,w,h,8,TFT_WHITE);

    tft.setTextColor(TFT_WHITE,color);

    tft.setTextSize(2);

    tft.setCursor(x+12,y+9);

    tft.print(text);
}



//====================================================
// HEADER
//====================================================

void drawHeader()
{
  int lcdW = tft.width(); 
    // Header putih
    tft.fillRect(0, 0, tft.width(), 30, TFT_WHITE);

    // Garis bawah
    tft.drawFastHLine(0, 30, tft.width(), TFT_LIGHTGREY);

    // Logo BRIN
lcdW = tft.width(); 

tft.pushImage(5,0,LOGO_WIDTH,LOGO_HEIGHT,logoBRIN);

tft.pushImage(30,0,LOGO_WIDTH,LOGO_HEIGHT,logoLPDP);

tft.pushImage(lcdW-65,0,LOGO_WIDTH,LOGO_HEIGHT,logoBLUETOOTH);

tft.pushImage(lcdW-35,0,LOGO_WIDTH,LOGO_HEIGHT,logoWIFI);

    // Judul
    tft.setTextColor(TFT_BLUE, TFT_WHITE);
    tft.setTextSize(2);
   lcdW = tft.width();

String judul = "ANTHROPOMETRY";

int x = (lcdW - tft.textWidth(judul)) / 2;

tft.setCursor(x,8);

tft.print(judul);
}

//====================================================
// BERAT
//====================================================

void drawBerat()
{
    int lcdW = tft.width();

    tft.setTextColor(TFT_WHITE, BG_COLOR);
    tft.setTextSize(2);
    tft.setCursor(10, 42);
    tft.print("BERAT");

    // Kotak mengikuti lebar layar
    tft.drawRoundRect(10, 60, lcdW - 20, 50, 6, TFT_DARKGREY);

    tft.setTextColor(BERAT_COLOR, BG_COLOR);
    tft.setTextSize(4);

    if (rotationTFT == 0)
    {
        // Landscape
        tft.setCursor(70, 72);
    }
    else
    {
        // Portrait
        tft.setCursor(35, 72);
    }

    tft.print(beratBadan, 2);

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, BG_COLOR);

    // Satuan selalu di kanan
    tft.setCursor(lcdW - 32, 84);
    tft.print("kg");
}
void updateBerat(float nilai)
{
    if (nilai == lastBerat)
        return;

    lastBerat = nilai;

    int lcdW = tft.width();
    int xAngka;

    if (rotationTFT == 0)
        xAngka = 70;
    else
        xAngka = 35;

    // Hapus area angka
    tft.fillRect(xAngka, 72, lcdW - 90, 32, TFT_BLACK);

    // Gambar angka baru
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(4);
    tft.setCursor(xAngka, 72);
    tft.print(nilai, 2);

    // Gambar lagi satuan
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(lcdW - 32, 84);
    tft.print("kg");
}

//====================================================
// TINGGI
//====================================================
void drawTinggi()
{
    int lcdW = tft.width();

    tft.setTextColor(TFT_WHITE, BG_COLOR);
    tft.setTextSize(2);
    tft.setCursor(10, 120);
    tft.print("TINGGI");

    // Kotak mengikuti lebar layar
    tft.drawRoundRect(10, 138, lcdW - 20, 50, 6, TFT_DARKGREY);

    tft.setTextColor(TINGGI_COLOR, BG_COLOR);
    tft.setTextSize(4);

    if(rotationTFT == 0)
    {
        // Portrait
        tft.setCursor(70,150);
    }
    else
    {
        // Landscape
        tft.setCursor(35,150);
    }

    tft.print(tinggiBadan,1);

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE,BG_COLOR);

    tft.setCursor(lcdW-32,162);
    tft.print("cm");

    tft.drawFastHLine(0,198,lcdW,TFT_DARKGREY);
}
void updateTinggi(float nilai)
{
    if(nilai == lastTinggi)
        return;

    lastTinggi = nilai;

    int lcdW = tft.width();

    int xAngka;

    if(rotationTFT==0)
        xAngka=70;
    else
        xAngka=35;

    tft.fillRect(xAngka,150,lcdW-90,32,TFT_BLACK);

    tft.setTextColor(TFT_GREEN,TFT_BLACK);
    tft.setTextSize(4);
    tft.setCursor(xAngka,150);
    tft.print(nilai,1);

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE,TFT_BLACK);
    tft.setCursor(lcdW-32,162);
    tft.print("cm");
}
//====================================================
// BUTTONS
//====================================================

void drawButtons()
{
    drawTareBeratButton(false);
    drawTareTinggiButton(false);
}
void drawTareBeratButton(bool tekan)
{
    uint16_t warna = tekan ? TFT_YELLOW : TFT_BLUE;
    uint16_t warnaText = tekan ? TFT_BLACK : TFT_WHITE;

    int x, y, w, h;

    if (rotationTFT == 0)
    {
        // Portrait
        x = 10;
        y = 205;
        w = 145;
        h = 30;
    }
    else
    {
        // Landscape
        x = 20;
        y = 205;
        w = 145;
        h = 30;
    }

    tft.fillRoundRect(x, y, w, h, 8, warna);
    tft.drawRoundRect(x, y, w, h, 8, TFT_WHITE);

    tft.setTextColor(warnaText, warna);
    tft.setTextSize(2);

    String txt = "TARE BERAT";

    int tx = x + (w - tft.textWidth(txt)) / 2;
    int ty = y + (h - 16) / 2;

    tft.setCursor(tx, ty);
    tft.print(txt);
}
void drawTareTinggiButton(bool tekan)
{
    uint16_t warna = tekan ? TFT_YELLOW : TFT_GREEN;
    uint16_t warnaText = tekan ? TFT_BLACK : TFT_WHITE;

    int x, y, w, h;

    if (rotationTFT == 0)
    {
        
        // Portrait
        x = 10;
        y = 255;
        w = 145;
        h = 30;
  
    }
    else
    {
        // Landscape
        x = 175;
        y = 205;
        w = 145;
        h = 30;
    }

    tft.fillRoundRect(x, y, w, h, 8, warna);
    tft.drawRoundRect(x, y, w, h, 8, TFT_WHITE);

    tft.setTextColor(warnaText, warna);
    tft.setTextSize(2);

    String txt = "TARE TINGGI";

    int tx = x + (w - tft.textWidth(txt)) / 2;
    int ty = y + (h - 16) / 2;

    tft.setCursor(tx, ty);
    tft.print(txt);
}
void tareTinggi()
{
    drawTareTinggiButton(true);

    delay(150);

    drawTareTinggiButton(false);

    Serial.println("TARE TINGGI");
}

//====================================================
// GUI
//====================================================

void initGUI()
{
    tft.init();

    tft.setRotation(1);

    tft.fillScreen(BG_COLOR);

    drawHeader();

    drawBerat();

    drawTinggi();

    drawButtons();
}

//====================================================
// TOUCH
//====================================================

void initTouch()
{
    touchSPI.begin(25,39,32,33);

    ts.begin(touchSPI);

    ts.setRotation(1);

    Serial.println("Touch Ready");
}


void tareBerat()
{
    drawTareBeratButton(true);

    delay(150);

    drawTareBeratButton(false);

    Serial.println("TARE BERAT");
}

void checkRotation()
{
    if (rotationTFT == currentRotation)
        return;

    currentRotation = rotationTFT;

    if (rotationTFT == 0)
        tft.setRotation(2);
    else
        tft.setRotation(1);

    Serial.printf("Rotation=%d  LCD=%d x %d\n",
                  rotationTFT,
                  tft.width(),
                  tft.height());

    updateLayout();

    drawGUI();
}




void drawGUI()
{
    tft.fillScreen(BG_COLOR);

    drawHeader();
    drawBerat();
    drawTinggi();
    drawButtons();
}

void updateLayout()
{
    if(rotationTFT == 0)
    {
        // ==========================
        // TIDUR (Landscape 320x240)
        // ==========================
        BTN1_X = 10;
        BTN1_Y = 205;
        BTN1_W = 145;
        BTN1_H = 30;

        BTN2_X = 165;
        BTN2_Y = 205;
        BTN2_W = 145;
        BTN2_H = 30;
    }
    else
    {
        // ==========================
        // BERDIRI (Portrait 240x320)
        // ==========================
        BTN1_X = 20;
        BTN1_Y = 215;
        BTN1_W = 200;
        BTN1_H = 35;

        BTN2_X = 20;
        BTN2_Y = 250;
        BTN2_W = 200;
        BTN2_H = 35;
    }

    Serial.println("===== UPDATE LAYOUT =====");
    Serial.printf("Rotation : %d\n", rotationTFT);
    Serial.printf("BTN1 : X=%d Y=%d W=%d H=%d\n",
                  BTN1_X, BTN1_Y, BTN1_W, BTN1_H);
    Serial.printf("BTN2 : X=%d Y=%d W=%d H=%d\n",
                  BTN2_X, BTN2_Y, BTN2_W, BTN2_H);
}

//====================================================
// READ TOUCH
//====================================================

void readTouch()
{
    if (ts.touched())
    {
        TS_Point p = ts.getPoint();

        touchX = getTouchX(p.x);
        touchY = getTouchY(p.y);

        Serial.print("RAW X=");
        Serial.print(p.x);

        Serial.print("  RAW Y=");
        Serial.print(p.y);

        Serial.print("   -->   ");

        Serial.print(touchX);
        Serial.print(",");
        Serial.println(touchY);

        //========================================
        // TOMBOL TARE BERAT
        //========================================

        if (touchX >= BTN1_X &&
            touchX <= BTN1_X + BTN1_W &&
            touchY >= BTN1_Y &&
            touchY <= BTN1_Y + BTN1_H)
        {
            //Serial.println("===== TARE BERAT =====");
            tareBerat();
        }
        if (touchX >= BTN2_X &&
        touchX <= BTN2_X + BTN2_W &&
        touchY >= BTN2_Y &&
        touchY <= BTN2_Y + BTN2_H)
        {
            tareTinggi();
        }

        //========================================
        // TOMBOL TARE TINGGI
        //========================================

        if (touchX >= BTN2_X &&
            touchX <= BTN2_X + BTN2_W &&
            touchY >= BTN2_Y &&
            touchY <= BTN2_Y + BTN2_H)
        {
            Serial.println("===== TARE TINGGI =====");
        }

        delay(200);
    }
}

//====================================================
// SETUP
//====================================================

void setup()
{
    Serial.begin(115200);
   tft.setSwapBytes(true);

    randomSeed(micros());
  rotationTFT = 0;
  initGUI();

  updateLayout();

  drawGUI();

    initTouch();
    updateBerat(beratBadan);
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



//====================================================
// LOOP
//====================================================

//void loop()
//{
//    readTouch();
//}


    unsigned long lastUpdate = 0;

void loop()
{
  checkRotation();
    readTouch();
    
    if (millis() - lastUpdate >= 1000)
    {
        lastUpdate = millis();

        //float berat = random(5000, 9000) / 100.0;

        //setBerat(nilai);
         updateBerat(beratBadan);
         updateTinggi(tinggiBadan);
    }
if (Serial.available())
{
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
//================== VL53 A ==================

if (cmd.startsWith("VL53A="))
{
    float kal = cmd.substring(6).toFloat();

    sendData.nodeID = 78;
    sendData.packetID = 0;
    sendData.value = kal;
    sendData.rotationID = 0;

    esp_err_t result = esp_now_send(macVL53,
                                    (uint8_t *)&sendData,
                                    sizeof(sendData));

    Serial.print("Kirim A VL53 : ");
    Serial.println(kal);

    if (result == ESP_OK)
        Serial.println("Kirim OK");
    else
        Serial.println("Kirim GAGAL");
}

//================== VL53 B ==================

if (cmd.startsWith("VL53B="))
{
    float kal = cmd.substring(6).toFloat();

    sendData.nodeID = 79;
    sendData.packetID = 0;
    sendData.value = kal;
    sendData.rotationID = 0;

    esp_err_t result = esp_now_send(macVL53,
                                    (uint8_t *)&sendData,
                                    sizeof(sendData));

    Serial.print("Kirim B VL53 : ");
    Serial.println(kal);

    if (result == ESP_OK)
        Serial.println("Kirim OK");
    else
        Serial.println("Kirim GAGAL");
}

  //================== HX711 A ==================

if (cmd.startsWith("HX711A="))
{
    float kal = cmd.substring(7).toFloat();

    sendData.nodeID = 88;
    sendData.packetID = 0;
    sendData.value = kal;
    sendData.rotationID = 0;

    esp_err_t result = esp_now_send(macHX711,
                                    (uint8_t *)&sendData,
                                    sizeof(sendData));

    Serial.print("Kirim A HX711 : ");
    Serial.println(kal);

    if (result == ESP_OK)
        Serial.println("Kirim OK");
    else
        Serial.println("Kirim GAGAL");
}

//================== HX711 B ==================

if (cmd.startsWith("HX711B="))
{
    float kal = cmd.substring(7).toFloat();

    sendData.nodeID = 89;
    sendData.packetID = 0;
    sendData.value = kal;
    sendData.rotationID = 0;

    esp_err_t result = esp_now_send(macHX711,
                                    (uint8_t *)&sendData,
                                    sizeof(sendData));

    Serial.print("Kirim B HX711 : ");
    Serial.println(kal);

    if (result == ESP_OK)
        Serial.println("Kirim OK");
    else
        Serial.println("Kirim GAGAL");
}

//================== HX711 C ==================

if (cmd.startsWith("HX711C="))
{
    float kal = cmd.substring(7).toFloat();

    sendData.nodeID = 90;
    sendData.packetID = 0;
    sendData.value = kal;
    sendData.rotationID = 0;

    esp_err_t result = esp_now_send(macHX711,
                                    (uint8_t *)&sendData,
                                    sizeof(sendData));

    Serial.print("Kirim C HX711 : ");
    Serial.println(kal);

    if (result == ESP_OK)
        Serial.println("Kirim OK");
    else
        Serial.println("Kirim GAGAL");
}
}

}