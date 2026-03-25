#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <U8g2lib.h>

// OLED SH1106
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

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
   MAC GATEWAY
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
   OLED DISPLAY
============================== */
void updateOLED()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  char line[32];

  sprintf(line,"NODE : %d",data.nodeID);
  u8g2.drawStr(0,15,line);

  sprintf(line,"PKT  : %lu",data.packetID);
  u8g2.drawStr(0,30,line);

  sprintf(line,"BB : %.2f kg",data.value);
  u8g2.drawStr(0,50,line);

  u8g2.sendBuffer();
}

/* ==============================
   SETUP
============================== */
void setup()
{
  Serial.begin(115200);

  /* OLED START */
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(10,30,"NODE 1 START");
  u8g2.sendBuffer();
  delay(1500);

  /* WIFI INIT (PENTING) */
  WiFi.mode(WIFI_STA);
  WiFi.STA.begin();   // penting di board kamu
  delay(100);

  // paksa channel sama dengan gateway (channel 1)
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  Serial.print("MAC NODE: ");
  Serial.println(WiFi.macAddress());

  /* ESP NOW INIT */
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP NOW ERROR");
    return;
  }

  // register callback status kirim
  esp_now_register_send_cb(OnDataSent);

  // set peer (gateway)
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Gagal tambah peer");
    return;
  }

  Serial.println("SENDER SIAP 🚀");
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

    // simulasi berat badan
    data.value = random(500,900) / 10.0;

    // kirim data
    esp_err_t result = esp_now_send(
      broadcastAddress,
      (uint8_t*)&data,
      sizeof(data)
    );

    Serial.print("Send BB : ");
    Serial.println(data.value);

    if (result == ESP_OK)
      Serial.println("Queue OK");
    else
      Serial.println("Queue ERROR");

    updateOLED();
  }
}