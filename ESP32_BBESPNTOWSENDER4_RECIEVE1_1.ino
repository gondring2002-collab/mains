#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include <Wire.h>
#include <VL53L1X.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Preferences.h>
/* ==============================
   VL53L1X CONFIG
============================== */
VL53L1X tof;
/* ==============================
  MPU6050 CONFIG
============================== */
Adafruit_MPU6050 mpu;

Preferences pref;

float kalA = 1.0;
float kalB = 0.0;

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
//--------------------------------------------------------------

/* ==============================
   SEND INTERVAL
============================== */
uint32_t sendInterval = 700;
uint32_t lastSend = 0;

/* ==============================
   MAC GATEWAY
============================== */
uint8_t broadcastAddress[] =
//{0xD4,0x8A,0xFC,0xA7,0xBD,0x20};
{0x88,0x57,0x21,0x2e,0x6e,0x4c};
esp_now_peer_info_t peerInfo;
const int ledPin = 2;   // LED onboard ESP32




void OnDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *incomingData,
                int len)
{
    if (len != sizeof(sensorPacket))
        return;

    sensorPacket dataMasuk;
    memcpy(&dataMasuk, incomingData, sizeof(sensorPacket));

    //=========================
    // KALIBRASI A
    //=========================
    if (dataMasuk.nodeID == 78)
    {
        kalA = dataMasuk.value;

        // Simpan ke Preferences
        pref.putFloat("A", kalA);

        Serial.println("========================");
        Serial.println("Kalibrasi A diterima");

        Serial.print("A baru : ");
        Serial.println(kalA);

        Serial.print("Isi EEPROM A : ");
        Serial.println(pref.getFloat("A"));
        Serial.println("========================");

        // ACK
        sensorPacket balas;

        balas.nodeID = 178;
        balas.packetID = 0;
        balas.value = kalA;
        balas.rotationID = 0;

        esp_now_send(broadcastAddress,
                     (uint8_t *)&balas,
                     sizeof(balas));
    }

    //=========================
    // KALIBRASI B
    //=========================
    if (dataMasuk.nodeID == 79)
    {
        kalB = dataMasuk.value;

        // Simpan ke Preferences
        pref.putFloat("B", kalB);

        Serial.println("========================");
        Serial.println("Kalibrasi B diterima");

        Serial.print("B baru : ");
        Serial.println(kalB);

        Serial.print("Isi EEPROM B : ");
        Serial.println(pref.getFloat("B"));
        Serial.println("========================");

        // ACK
        sensorPacket balas;

        balas.nodeID = 179;
        balas.packetID = 0;
        balas.value = kalB;
        balas.rotationID = 0;

        esp_now_send(broadcastAddress,
                     (uint8_t *)&balas,
                     sizeof(balas));
    }
    //=========================
// LIHAT NILAI KALIBRASI A
// ID 76
//=========================
//=========================
// MINTA NILAI A
// ID 76
// BALAS ID 176
//=========================
if (dataMasuk.nodeID == 76)
{
    float nilaiA = pref.getFloat("A", 1.0);

    sensorPacket balas;

    balas.nodeID = 176;
    balas.packetID = 0;
    balas.value = nilaiA;
    balas.rotationID = 0;

    esp_now_send(broadcastAddress,
                 (uint8_t *)&balas,
                 sizeof(balas));

    Serial.print("Balas nilai A : ");
    Serial.println(nilaiA);
}


//=========================
// LIHAT NILAI KALIBRASI B
// ID 77
//=========================
//=========================
// MINTA NILAI B
// ID 77
// BALAS ID 177
//=========================
if (dataMasuk.nodeID == 77)
{
    float nilaiB = pref.getFloat("B", 0.0);

    sensorPacket balas;

    balas.nodeID = 177;
    balas.packetID = 0;
    balas.value = nilaiB;
    balas.rotationID = 0;

    esp_now_send(broadcastAddress,
                 (uint8_t *)&balas,
                 sizeof(balas));

    Serial.print("Balas nilai B : ");
    Serial.println(nilaiB);
}
}



/* ==============================
   CALLBACK STATUS KIRIM
============================== */
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status)
{
  Serial.print("Kirim ke: ");

  for (int i = 0; i < 6; i++) {
    Serial.print(info->des_addr[i], HEX);
    if (i < 5) Serial.print(":");
  }

  Serial.print(" | Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}



/* ==============================
   SETUP
============================== */
void setup()
{
  Serial.begin(115200);
 pref.begin("kalibrasi", false);



Serial.print("Kalibrasi EEPROM = ");
kalA = pref.getFloat("A", 1.0);
kalB = pref.getFloat("B", 0.0);

Serial.print("A = ");
Serial.println(kalA);

Serial.print("B = ");
Serial.println(kalB);

 pinMode(ledPin, OUTPUT);
  /* VL53L1X INIT */
  //Wire.begin();
  Wire.begin(21, 22);
  if (!tof.init())
  {
    Serial.println("VL53L1X tidak ditemukan");
  }
  else
  {
    tof.setDistanceMode(VL53L1X::Long);
    tof.setMeasurementTimingBudget(50000);
    tof.startContinuous(50);

    Serial.println("VL53L1X siap");
  }

  if (!mpu.begin())
  {
    Serial.println("MPU6050 tidak ditemukan!");
    while (1);
  }

  Serial.println("MPU6050 siap.");


  /* WIFI INIT */
  WiFi.mode(WIFI_STA);
  WiFi.STA.begin();
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

  
}

/* ==============================
   LOOP
============================== */
void loop()
{
   digitalWrite(ledPin, LOW);  // LED mati
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  
  // Deteksi posisi PCB
  if (abs(a.acceleration.z) > abs(a.acceleration.y))
  {
      data.rotationID = 0;    // Tidur
  }
  else
  {
      data.rotationID = 1;    // Berdiri
  }

  if (millis() - lastSend >= sendInterval)
  {
    lastSend = millis();

    data.nodeID = 1;
    data.packetID = packet_counter++;
    // data.value = 5;//readLoadCell()*-1;
    //    data.value = tof.read();
    //data.value = tof.read() + tinggiOffset;

    float raw = tof.read();
data.value = kalA * raw + kalB;

    Serial.print("Rotation : ");
    Serial.println(data.rotationID);

    esp_err_t result = esp_now_send(
      broadcastAddress,
      (uint8_t*)&data,
      sizeof(data)
    );

    //Serial.print("Tinggi: ");
    //Serial.println(data.value);
    //Serial.print("Accel X: ");
    //Serial.print(a.acceleration.x);
    //Serial.print(" Y: ");
    //Serial.print(a.acceleration.y);
    //Serial.print(" Z: ");
    //Serial.println(a.acceleration.z);

    //Serial.print("Gyro X: ");
    //Serial.print(g.gyro.x);
    //Serial.print(" Y: ");
    //Serial.print(g.gyro.y);
    //Serial.print(" Z: ");
    //Serial.println(g.gyro.z);
  
    if (result == ESP_OK)
    {
      //Serial.println("Queue OK");
      digitalWrite(ledPin, HIGH); // LED menyala
      delay(500);
    }
    else
      Serial.println("Queue ERROR");
  }
}