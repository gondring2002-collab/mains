#include <esp_now.h> 
#include <WiFi.h>
 #include <esp_wifi.h>
 #include "HX711.h" 
 #include <Preferences.h>

 long tareOffset = 0;
 /* ============================== HX711 CONFIG ============================== */ 
 #define HX711_DOUT 15
 #define HX711_SCK 4
 HX711 scale;
  float kalA = -7050;   // calibration factor HX711
float kalB = 1.0;     // slope
float kalC = 0.0;     // offset
//tareOffset = pref.getLong("TARE", 0); 
  Preferences pref;
  /* ============================== DATA STRUCTURE ============================== */ 
  typedef struct __attribute__((packed)) 
  { 
  uint8_t nodeID; 
  uint32_t packetID; 
  float value;
  uint8_t rotationID; 
  } 
  sensorPacket;
   sensorPacket data; 
   uint32_t packet_counter = 0;
  const int ledPin = 2;   // LED onboard ESP32
/* ============================== SEND INTERVAL ============================== */ 
    uint32_t sendInterval = 700; uint32_t lastSend = 0;
     /* ============================== MAC GATEWAY ============================== */
      uint8_t broadcastAddress[] = 
      //{0xD4,0x8A,0xFC,0xA7,0xBD,0x20};
      {0x88,0x57,0x21,0x2e,0x6e,0x4c}; 
       esp_now_peer_info_t peerInfo; 
       /* ============================== CALLBACK STATUS KIRIM ============================== */ 


void OnDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *incomingData,
                int len)
{
    if (len != sizeof(sensorPacket))
        return;

    sensorPacket dataMasuk;
    memcpy(&dataMasuk, incomingData, sizeof(sensorPacket));

    sensorPacket balas;
    balas.packetID = 0;
    balas.rotationID = 0;

    //=========================
    // HX711 A (Calibration Factor)
    //=========================
    if (dataMasuk.nodeID == 88)
    {
        kalA = dataMasuk.value;

        pref.putFloat("A", kalA);

        // Terapkan ke HX711
        scale.set_scale(kalA);

        Serial.println("========================");
        Serial.println("Kalibrasi A diterima");
        Serial.print("A = ");
        Serial.println(kalA);
        Serial.println("========================");

        balas.nodeID = 188;
        balas.value = kalA;

        esp_now_send(broadcastAddress,
                     (uint8_t *)&balas,
                     sizeof(balas));
    }

    //=========================
    // HX711 B (Slope)
    //=========================
    if (dataMasuk.nodeID == 89)
    {
        kalB = dataMasuk.value;

        pref.putFloat("B", kalB);

        Serial.println("========================");
        Serial.println("Kalibrasi B diterima");
        Serial.print("B = ");
        Serial.println(kalB);
        Serial.println("========================");

        balas.nodeID = 189;
        balas.value = kalB;

        esp_now_send(broadcastAddress,
                     (uint8_t *)&balas,
                     sizeof(balas));
    }

    //=========================
    // HX711 C (Offset)
    //=========================
    if (dataMasuk.nodeID == 90)
    {
        kalC = dataMasuk.value;

        pref.putFloat("C", kalC);

        Serial.println("========================");
        Serial.println("Kalibrasi C diterima");
        Serial.print("C = ");
        Serial.println(kalC);
        Serial.println("========================");

        balas.nodeID = 190;
        balas.value = kalC;

        esp_now_send(broadcastAddress,
                     (uint8_t *)&balas,
                     sizeof(balas));
    }

    //=========================
// HX711 TARE
//========================
if (dataMasuk.nodeID == 84)
{
    Serial.println("========================");
    Serial.println("PERINTAH TARE DITERIMA");
    Serial.println("Melakukan TARE...");
    Serial.println("========================");

    scale.set_scale();
    scale.tare();

    Serial.println("========================");
    Serial.println("TARE SELESAI");
    Serial.println("========================");

    balas.nodeID = 191;
    balas.value = 1.0;

    esp_now_send(broadcastAddress,
                 (uint8_t *)&balas,
                 sizeof(balas));
  }
 //=========================
// MINTA NILAI A
// ID 85
// BALAS ID 185
//=========================

if (dataMasuk.nodeID == 85)
{
    float nilaiKalA = pref.getFloat("A", -7050);

    Serial.println("========================");
    Serial.println("MINTA NILAI HX711A_A");
    Serial.print("A = ");
    Serial.println(nilaiKalA, 6);
    Serial.println("========================");

    balas.nodeID = 185;
    balas.value = nilaiKalA;

    esp_now_send(broadcastAddress,
                 (uint8_t *)&balas,
                 sizeof(balas));
}
if (dataMasuk.nodeID == 86)
{
    float nilaiKalB = pref.getFloat("B", 1.0);

    Serial.println("========================");
    Serial.println("MINTA NILAI HX711A_B");
    Serial.print("B = ");
    Serial.println(nilaiKalB, 6);
    Serial.println("========================");

    balas.nodeID = 186;
    balas.value = nilaiKalB;

    esp_now_send(broadcastAddress,
                 (uint8_t *)&balas,
                 sizeof(balas));
}
if (dataMasuk.nodeID == 87)
{
    float nilaiKalC = pref.getFloat("C", 0.0);

    Serial.println("========================");
    Serial.println("MINTA NILAI HX711A_C");
    Serial.print("C = ");
    Serial.println(nilaiKalC, 6);
    Serial.println("========================");

    balas.nodeID = 187;
    balas.value = nilaiKalC;

    esp_now_send(broadcastAddress,
                 (uint8_t *)&balas,
                 sizeof(balas));
}



}



    
       void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status)
        { Serial.print("Kirim ke: ");
         for (int i = 0; i < 6; i++) 
        { Serial.print(info->des_addr[i], HEX); if (i < 5) Serial.print(":");
         } 
         Serial.print(" | Status: "); 
         Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL"); 
         } /* ============================== READ LOADCELL (STABIL TANPA ERROR) ============================== */
float readLoadCell() { float total = 0; 
int valid = 0; 
for(int i=0;i<10;i++) 
{ float val = scale.get_units(1); // filter nilai aneh 
if (!isnan(val) && val > -1000 && val < 1000) 
{ total += val; valid++;
 } delay(5); } 
 if (valid == 0) { Serial.println("DATA ERROR"); return 0; } return total / valid; 
 } 
 
 /* ============================== SETUP ============================== */ 
 void setup() 
 {
 Serial.begin(115200); 
   digitalWrite(ledPin, LOW);  // LED mati

pref.begin("hx711", false);

kalA = pref.getFloat("A", -7050);
kalB = pref.getFloat("B", 1.0);
kalC = pref.getFloat("C", 0.0);
 pinMode(ledPin, OUTPUT);
Serial.println("========================");
Serial.print("Kalibrasi EEPROM : ");
//Serial.println(calibration_factor);
Serial.println("========================");


 /* HX711 INIT */ 
 scale.begin(HX711_DOUT, HX711_SCK); 
 if (!scale.is_ready())
{
    Serial.println("ERROR : HX711 TIDAK TERDETEKSI");

    while (1)
    {
        digitalWrite(ledPin, !digitalRead(ledPin));
        delay(300);
    }
}

Serial.println("HX711 OK");
 delay(500); 
 // penting biar stabil 
scale.set_scale(kalA);
 scale.tare(); 
 Serial.println("Tare done"); 
 /* WIFI INIT */ 
 WiFi.mode(WIFI_STA); 
 WiFi.STA.begin(); delay(100); 
 // paksa channel sama dengan gateway esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE); 
 Serial.print("MAC NODE: "); 
 Serial.println(WiFi.macAddress()); 
 /* ESP NOW INIT */ 
 if (esp_now_init() != ESP_OK) 
 { 
  Serial.println("ESP NOW ERROR"); 
  return; 
  } 
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv); 
  /* SET PEER */ 
  memcpy(peerInfo.peer_addr, broadcastAddress, 6); 
  peerInfo.channel = 1; 
  peerInfo.encrypt = false; 
  if (esp_now_add_peer(&peerInfo) != ESP_OK) 
  { 
    Serial.println("Gagal tambah peer"); 
    return; 
  } 
    Serial.println("SENDER LOADCELL SIAP"); 
    }
    
 /* ============================== LOOP ============================== */ 
    
    void loop() 
    
    { 
       digitalWrite(ledPin, LOW);  // LED mati
      if (millis() - lastSend >= sendInterval) 
      { lastSend = millis(); 
        data.nodeID = 2; 
        data.packetID = packet_counter++; 

        float raw = readLoadCell() * -1;
        data.value = kalB * raw + kalC;

         digitalWrite(ledPin, HIGH);  // LED mati
        esp_err_t result = esp_now_send( broadcastAddress, (uint8_t*)&data, sizeof(data) ); 
        Serial.print("Berat: "); 
        Serial.println(data.value); 
        if (result == ESP_OK) 
        Serial.println("Queue OK"); 
        else 
        Serial.println("Queue ERROR"); 
      } 
    }



    