/*******************************************************
 * GUI-06
 * CYD ESP32-2432S028
 * TFT_eSPI + XPT2046_Touchscreen
 *******************************************************/

#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "logoBRIN.h"
#include "logoLPDP.h"
//====================================================
// TFT
//====================================================

TFT_eSPI tft = TFT_eSPI();

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

#define BTN1_X   10
#define BTN1_Y   205
#define BTN1_W   145
#define BTN1_H   30

#define BTN2_X   165
#define BTN2_Y   205
#define BTN2_W   145
#define BTN2_H   30

//====================================================
// DATA DEMO
//====================================================

float berat = 68.25;
float tinggi = 172.4;
float lastBerat = -999;
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
    // Header putih
    tft.fillRect(0, 0, 320, 30, TFT_WHITE);

    // Garis bawah
    tft.drawFastHLine(0, 30, 320, TFT_LIGHTGREY);

    // Logo BRIN
    tft.pushImage(
        5,
        0,
        LOGO_WIDTH,
        LOGO_HEIGHT,
        logoBRIN
    );

    // Logo LPDP
    tft.pushImage(
        285,
        0,
        LOGO_WIDTH,
        LOGO_HEIGHT,
        logoLPDP
    );

    // Judul
    tft.setTextColor(TFT_BLUE, TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(90, 8);
    tft.print("HELLO GIGIN");
}

//====================================================
// BERAT
//====================================================

void drawBerat()
{
    tft.setTextColor(TFT_WHITE,BG_COLOR);

    tft.setTextSize(2);

    tft.setCursor(10,42);

    tft.print("BERAT");

    tft.drawRoundRect(10,60,300,50,6,TFT_DARKGREY);

    tft.setTextColor(BERAT_COLOR,BG_COLOR);

    tft.setTextSize(4);

    tft.setCursor(70,72);

    tft.print(berat,2);

    tft.setTextSize(2);

    tft.setTextColor(TFT_WHITE,BG_COLOR);

    tft.setCursor(250,84);

    tft.print("kg");
}
void updateBerat(float nilai)
{
    if (nilai == lastBerat)
        return;

    lastBerat = nilai;

    // Hapus area angka saja
    tft.fillRect(70,72,150,32,TFT_BLACK);

    // Tulis angka baru
    tft.setTextColor(TFT_YELLOW,TFT_BLACK);
    tft.setTextSize(4);
    tft.setCursor(70,72);
    tft.print(nilai,2);
}
//====================================================
// TINGGI
//====================================================

void drawTinggi()
{
    tft.setTextColor(TFT_WHITE,BG_COLOR);

    tft.setTextSize(2);

    tft.setCursor(10,120);

    tft.print("TINGGI");

    tft.drawRoundRect(10,138,300,50,6,TFT_DARKGREY);

    tft.setTextColor(TINGGI_COLOR,BG_COLOR);

    tft.setTextSize(4);

    tft.setCursor(70,150);

    tft.print(tinggi,1);

    tft.setTextSize(2);

    tft.setTextColor(TFT_WHITE,BG_COLOR);

    tft.setCursor(250,162);

    tft.print("cm");

    tft.drawFastHLine(0,198,320,TFT_DARKGREY);
}

//====================================================
// BUTTONS
//====================================================
/*void drawButtons()
{
    drawButton(BTN1_X,
               BTN1_Y,
               BTN1_W,
               BTN1_H,
               BTN1_COLOR,
               "TARE BERAT");

    drawButton(BTN2_X,
               BTN2_Y,
               BTN2_W,
               BTN2_H,
               BTN2_COLOR,
               "TARE TINGGI");
}*/
void drawButtons()
{
    drawTareButton(false);
    drawTareTinggiButton(false);
}
void drawTareTinggiButton(bool tekan)
{
    uint16_t warna;
    uint16_t warnaText;

    if(tekan)
    {
        warna = TFT_YELLOW;
        warnaText = TFT_BLACK;
    }
    else
    {
        warna = TFT_GREEN;
        warnaText = TFT_WHITE;
    }

    tft.fillRoundRect(
        BTN2_X,
        BTN2_Y,
        BTN2_W,
        BTN2_H,
        8,
        warna);

    tft.drawRoundRect(
        BTN2_X,
        BTN2_Y,
        BTN2_W,
        BTN2_H,
        8,
        TFT_WHITE);

    tft.setTextColor(warnaText, warna);
    tft.setTextSize(2);
    tft.setCursor(BTN2_X + 8, BTN2_Y + 9);
    tft.print("TARE TINGGI");
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
void drawTareButton(bool tekan)
{
    uint16_t warna;
    uint16_t warnaText;

    if(tekan)
    {
        warna = TFT_YELLOW;
        warnaText = TFT_BLACK;
    }
    else
    {
        warna = TFT_BLUE;
        warnaText = TFT_WHITE;
    }

    tft.fillRoundRect(
        BTN1_X,
        BTN1_Y,
        BTN1_W,
        BTN1_H,
        8,
        warna);

    tft.drawRoundRect(
        BTN1_X,
        BTN1_Y,
        BTN1_W,
        BTN1_H,
        8,
        TFT_WHITE);

    tft.setTextColor(warnaText, warna);
    tft.setTextSize(2);

    tft.setCursor(BTN1_X + 12, BTN1_Y + 9);
    tft.print("TARE BERAT");
}

void tareBerat()
{
    drawTareButton(true);

    delay(150);

    drawTareButton(false);

    Serial.println("TARE BERAT");
}


//====================================================
// SETUP
//====================================================

void setup()
{
    Serial.begin(115200);
    tft.setSwapBytes(true);
    randomSeed(micros());
    initGUI();

    initTouch();
    updateBerat(berat);

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
// LOOP
//====================================================

//void loop()
//{
//    readTouch();
//}


    unsigned long lastUpdate = 0;

void loop()
{
    readTouch();

    if (millis() - lastUpdate >= 1000)
    {
        lastUpdate = millis();

        float berat = random(5000, 9000) / 100.0;

        //setBerat(nilai);
         updateBerat(berat);
    }

}