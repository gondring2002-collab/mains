#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "HX711.h"


/* ==============================
   HX711 CONFIG
============================== */
#define HX711_DOUT  4
#define HX711_SCK   16

HX711 scale;
float calibration_factor = -7050; // sesuaikan nanti

/* ==============================
   DATA STRUCTURE
============================== */
typedef struct __attribute__((packed))
{
  uint8_t nodeID;
  uint32_t packetID;
  float value;
} sensorPacket;

sensorPacket data;

uint32_t packet_counter = 0;

/* ==============================
   SEND INTERVAL
============================== */
uint32_t sendInterval = 700;
uint32_t lastSend = 0;

/* ==============================
   MAC GATEWAY (GANTI JIKA PERLU)
============================== */
uint8_t broadcastAddress[] =
{0xD4,0x8A,0xFC,0xA7,0xBD,0x20};

esp_now_peer_info_t peerInfo;

/* ==============================
   CALLBACK STATUS KIRIM (CORE 3.x)
============================== */
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status)
{
  Serial.print("Kirim ke: ");

  for (int i = 0; i < 6; i++) {
    Serial.print(info->des_addr[i], HEX);
    if (i < 5) Serial.print(":");
  }

  Serial.print(" | Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK ✅" : "FAIL ❌");
}

/* ==============================
   READ LOADCELL (STABIL)
============================== */
float readLoadCell()
{
  if (scale.is_ready())
  {
    float total = 0;

    for(int i=0;i<10;i++)
    {
      total += scale.get_units(1);
    }

    return total / 10.0;
  }
  else
  {
    Serial.println("HX711 not found");
    return 0;
  }
}

/* ==============================
   SETUP
============================== */
void setup()
{
  Serial.begin(115200);

  /* HX711 INIT */
  scale.begin(HX711_DOUT, HX711_SCK);
  scale.set_scale(calibration_factor);
  scale.tare();

  Serial.println("Tare done");

  /* WIFI INIT */
  WiFi.mode(WIFI_STA);
  WiFi.STA.begin();   // penting di beberapa board
  delay(100);

  // paksa channel sama dengan gateway
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  Serial.print("MAC NODE: ");
  Serial.println(WiFi.macAddress());

  /* ESP NOW INIT */
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP NOW ERROR");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  /* SET PEER */
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Gagal tambah peer");
    return;
  }

  Serial.println("SENDER LOADCELL SIAP 🚀");
}

/* ==============================
   LOOP
============================== */
void loop()
{
  if (millis() - lastSend >= sendInterval)
  {
    lastSend = millis();

    data.nodeID = 1;
    data.packetID = packet_counter++;
    data.value = readLoadCell();

    esp_err_t result = esp_now_send(
      broadcastAddress,
      (uint8_t*)&data,
      sizeof(data)
    );

    Serial.print("Berat: ");
    Serial.println(data.value);

    if (result == ESP_OK)
      Serial.println("Queue OK");
    else
      Serial.println("Queue ERROR");
  }
}
