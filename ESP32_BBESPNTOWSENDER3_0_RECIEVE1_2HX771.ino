#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "HX711.h"
#include <Preferences.h>

/* ==============================
   HX711 CONFIG
============================== */

// HX711 1
#define HX7111_DOUT 15
#define HX7111_SCK   4

// HX711 2
#define HX7112_DOUT 16
#define HX7112_SCK  17

HX711 scale1;
HX711 scale2;

float calibration_factor1 = -7050;
float calibration_factor2 = -7050;

Preferences pref;

/* ==============================
   DATA STRUCTURE
============================== */

typedef struct __attribute__((packed))
{
    uint8_t nodeID;
    uint32_t packetID;
    float value;
    uint8_t rotationID;

} sensorPacket;

sensorPacket data;

uint32_t packet_counter = 0;

const int ledPin = 2;

/* ==============================
   SEND INTERVAL
============================== */

uint32_t sendInterval = 700;
uint32_t lastSend = 0;

/* ==============================
   MAC GATEWAY
============================== */

uint8_t broadcastAddress[] =
{
    0x88,0x57,0x21,0x2E,0x6E,0x4C
};

esp_now_peer_info_t peerInfo;

/* ==============================
   TERIMA DATA
============================== */

void OnDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *incomingData,
                int len)
{
    if (len != sizeof(sensorPacket))
        return;

    sensorPacket dataMasuk;

    memcpy(&dataMasuk,
           incomingData,
           sizeof(sensorPacket));

    // Kalibrasi HX711-1
    if (dataMasuk.nodeID == 99)
    {
        calibration_factor1 = dataMasuk.value;

        pref.putFloat("kal1",
                      calibration_factor1);

        scale1.set_scale(calibration_factor1);

        Serial.println("========================");
        Serial.println("Kalibrasi HX711-1");
        Serial.println(calibration_factor1);
    }

    // Kalibrasi HX711-2
    if (dataMasuk.nodeID == 98)
    {
        calibration_factor2 = dataMasuk.value;

        pref.putFloat("kal2",
                      calibration_factor2);

        scale2.set_scale(calibration_factor2);

        Serial.println("========================");
        Serial.println("Kalibrasi HX711-2");
        Serial.println(calibration_factor2);
    }

    // ACK
    if (dataMasuk.nodeID == 99 ||
        dataMasuk.nodeID == 98)
    {
        sensorPacket balas;

        balas.nodeID = 199;
        balas.packetID = 0;
        balas.value = dataMasuk.value;
        balas.rotationID = 0;

        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas));
    }
}

/* ==============================
   CALLBACK KIRIM
============================== */

void OnDataSent(const wifi_tx_info_t *info,
                esp_now_send_status_t status)
{
    Serial.print("Kirim ke : ");

    for (int i = 0; i < 6; i++)
    {
        Serial.print(info->des_addr[i], HEX);

        if (i < 5)
            Serial.print(":");
    }

    Serial.print(" Status : ");

    Serial.println(
        status == ESP_NOW_SEND_SUCCESS ?
        "OK" : "FAIL");
}

/* ==============================
   READ HX711-1
============================== */

float readLoadCell1()
{
    float total = 0;
    int valid = 0;

    for (int i = 0; i < 10; i++)
    {
        float val = scale1.get_units(1);

        if (!isnan(val) &&
            val > -1000 &&
            val < 1000)
        {
            total += val;
            valid++;
        }

        delay(5);
    }

    if (valid == 0)
    {
        Serial.println("HX711-1 ERROR");
        return 0;
    }

    return total / valid;
}

/* ==============================
   READ HX711-2
============================== */

float readLoadCell2()
{
    float total = 0;
    int valid = 0;

    for (int i = 0; i < 10; i++)
    {
        float val = scale2.get_units(1);

        if (!isnan(val) &&
            val > -1000 &&
            val < 1000)
        {
            total += val;
            valid++;
        }

        delay(5);
    }

    if (valid == 0)
    {
        Serial.println("HX711-2 ERROR");
        return 0;
    }

    return total / valid;
}

/* ==============================
   SETUP
============================== */

void setup()
{
    Serial.begin(115200);

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    pref.begin("hx711", false);

    calibration_factor1 = pref.getFloat("kal1", -7050);
    calibration_factor2 = pref.getFloat("kal2", -7050);

    Serial.println("========================");
    Serial.print("Kalibrasi HX711-1 : ");
    Serial.println(calibration_factor1);

    Serial.print("Kalibrasi HX711-2 : ");
    Serial.println(calibration_factor2);
    Serial.println("========================");

    /* HX711 */

    scale1.begin(HX7111_DOUT, HX7111_SCK);
    scale2.begin(HX7112_DOUT, HX7112_SCK);

    delay(500);

    scale1.set_scale(calibration_factor1);
    scale2.set_scale(calibration_factor2);

    scale1.tare();
    scale2.tare();

    Serial.println("Tare Done");

    /* WIFI */

    WiFi.mode(WIFI_STA);
    WiFi.STA.begin();

    delay(100);

    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    Serial.print("MAC NODE : ");
    Serial.println(WiFi.macAddress());

    /* ESP NOW */

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ESP NOW ERROR");
        return;
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    memcpy(peerInfo.peer_addr,
           broadcastAddress,
           6);

    peerInfo.channel = 1;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Tambah Peer Gagal");
        return;
    }

    Serial.println("2 HX711 READY");
}

/* ==============================
   LOOP
============================== */

void loop()
{
    digitalWrite(ledPin, LOW);

    if (millis() - lastSend >= sendInterval)
    {
        lastSend = millis();

        float berat1 = readLoadCell1();
        float berat2 = readLoadCell2();

        float totalBerat = berat1 + berat2;

        data.nodeID = 3;
        data.packetID = packet_counter++;
        data.rotationID = 0;

        data.value = totalBerat * -1;

        digitalWrite(ledPin, HIGH);

        esp_err_t result =
        esp_now_send(
            broadcastAddress,
            (uint8_t *)&data,
            sizeof(data));

        Serial.println("======================");
        Serial.print("HX711-1 : ");
        Serial.println(berat1);

        Serial.print("HX711-2 : ");
        Serial.println(berat2);

        Serial.print("TOTAL   : ");
        Serial.println(totalBerat);

        Serial.print("KIRIM   : ");
        Serial.println(data.value);

        if(result == ESP_OK)
            Serial.println("Queue OK");
        else
            Serial.println("Queue ERROR");
    }
}