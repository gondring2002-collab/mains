#include <esp_now.h> 
#include <WiFi.h>
 #include <esp_wifi.h>
 #include "HX711.h" 
 #include <Preferences.h>
 /* ============================== HX711 CONFIG ============================== */ 
 #define HX711_DOUT 15
 #define HX711_SCK 4
 HX711 scale;
  float calibration_factor = -7050; 
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

    if (dataMasuk.nodeID == 99)
    {
        calibration_factor = dataMasuk.value;

        // Simpan ke Preferences
        pref.putFloat("kal", calibration_factor);

        // Terapkan ke HX711
        scale.set_scale(calibration_factor);

        Serial.println("========================");
        Serial.println("Kalibrasi baru diterima");
        Serial.print("Nilai : ");
        Serial.println(calibration_factor);

        Serial.print("EEPROM : ");
        Serial.println(pref.getFloat("kal"));
        Serial.println("========================");

        // Kirim ACK ke CYD
        sensorPacket balas;

        balas.nodeID = 199;
        balas.packetID = 0;
        balas.value = calibration_factor;
        balas.rotationID = 0;

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

calibration_factor = pref.getFloat("kal", -7050);
 pinMode(ledPin, OUTPUT);
Serial.println("========================");
Serial.print("Kalibrasi EEPROM : ");
Serial.println(calibration_factor);
Serial.println("========================");


 /* HX711 INIT */ 
 scale.begin(HX711_DOUT, HX711_SCK); 
 delay(500); 
 // penting biar stabil 
 scale.set_scale(calibration_factor); 
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
        data.value = readLoadCell()*-1; 
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



    