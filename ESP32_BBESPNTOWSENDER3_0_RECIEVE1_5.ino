
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "HX711.h"
#include <Preferences.h>


// ============================================================
// HX711 CONFIG
// ============================================================
#define HX711_DOUT 13
#define HX711_SCK  12

HX711 scale;


// ============================================================
// KALIBRASI
// ============================================================
// C = Calibration Factor
// A = Slope
// B = Offset
// ============================================================

float kalC = 20734.434;
float kalA = 1.0;
float kalB = 0.0;


// ============================================================
// TARE RAW
// ============================================================

long rawT = 0;


// ============================================================
// RAW SEBELUMNYA
// ============================================================

long raw_sebelumnya = 0;


// ============================================================
// PREFERENCES
// ============================================================

Preferences pref;

// =========================
// VARIABEL KALIBRASI SERIAL
// =========================

//long rawT = 0;       // RAW saat tare
long rawK = 0;       // RAW saat beban 10 kg

const float MASSA_KALIBRASI = 10.0;

float calibrationFactor = 20734.434;

// ============================================================
// DATA STRUCTURE ESP-NOW
// ============================================================

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


// ============================================================
// LED
// ============================================================

const int ledPin = 2;


// ============================================================
// SEND INTERVAL
// ============================================================

uint32_t sendInterval = 700;
uint32_t lastSend = 0;


// ============================================================
// MAC GATEWAY
// ============================================================

uint8_t broadcastAddress[] =
{
    0x88, 0x57, 0x21, 0x2E, 0x6E, 0x4C
};

esp_now_peer_info_t peerInfo;


// ============================================================
// HITUNG MASSA DARI RAW
// ============================================================

float hitungKg(long raw)
{
    if (kalC == 0)
        return 0;

    float kg = (float)(raw - rawT) / kalC;

    // A = slope
    // B = offset
    kg = kalA * kg + kalB;

    return kg;
}

// ============================================================
// KOMUNIKASI SERIAL T / K / C
// ============================================================

void prosesSerial()
{
    if (Serial.available())
    {
        char cmd = Serial.read();


        // ====================================================
        // T = TARE
        // ====================================================

        if (cmd == 'T' || cmd == 't')
        {
            rawT = scale.read();

            // Jadikan RAW TARE sebagai titik nol
            raw_sebelumnya = rawT;

            // Simpan ke Preferences
            pref.putLong("TARE", rawT);

            Serial.println();
            Serial.println("========== TARE ==========");

            Serial.print("RAW T = ");
            Serial.println(rawT);

            Serial.println("Massa = 0.0 kg");

            Serial.println("==========================");
            Serial.println();
        }


        // ====================================================
        // K = RAW BEBAN 10 KG
        // ====================================================

        else if (cmd == 'K' || cmd == 'k')
        {
            rawK = scale.read();

            Serial.println();
            Serial.println("======= BEBAN 10 KG =======");

            Serial.print("RAW K = ");
            Serial.println(rawK);

            Serial.print("Massa diketahui = ");
            Serial.print(MASSA_KALIBRASI, 1);
            Serial.println(" kg");

            Serial.println("===========================");
            Serial.println();
        }


        // ====================================================
        // C = HITUNG CALIBRATION FACTOR
        // ====================================================

        else if (cmd == 'C' || cmd == 'c')
        {
            long perubahanRaw = rawK - rawT;

            if (MASSA_KALIBRASI > 0)
            {
                calibrationFactor =
                    (float)perubahanRaw / MASSA_KALIBRASI;

                // Simpan sebagai C
                kalC = calibrationFactor;

                pref.putFloat("C", kalC);
            }

            Serial.println();
            Serial.println("====== HASIL KALIBRASI ======");

            Serial.print("RAW T = ");
            Serial.println(rawT);

            Serial.print("RAW K = ");
            Serial.println(rawK);

            Serial.print("Perubahan RAW = ");
            Serial.println(perubahanRaw);

            Serial.print("Massa = ");
            Serial.print(MASSA_KALIBRASI, 1);
            Serial.println(" kg");

            Serial.print("CALIBRATION FACTOR = ");
            Serial.println(calibrationFactor, 3);

            Serial.println("=============================");
            Serial.println();
        }
    }
}

// ============================================================
// CALLBACK ESP-NOW RECEIVE
// ============================================================

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


    sensorPacket balas;

    balas.packetID = 0;
    balas.rotationID = 0;


    // ========================================================
    // HX711 A
    // ID 88
    // ========================================================

    if (dataMasuk.nodeID == 88)
    {
        kalA = dataMasuk.value;

        pref.putFloat("A", kalA);

        Serial.println();
        Serial.println("========================");
        Serial.println("KALIBRASI A DITERIMA");
        Serial.print("A = ");
        Serial.println(kalA, 6);
        Serial.println("========================");


        balas.nodeID = 188;
        balas.value = kalA;

        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas)
        );
    }


    // ========================================================
    // HX711 B
    // ID 89
    // ========================================================

    if (dataMasuk.nodeID == 89)
    {
        kalB = dataMasuk.value;

        pref.putFloat("B", kalB);

        Serial.println();
        Serial.println("========================");
        Serial.println("KALIBRASI B DITERIMA");
        Serial.print("B = ");
        Serial.println(kalB, 6);
        Serial.println("========================");


        balas.nodeID = 189;
        balas.value = kalB;

        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas)
        );
    }


    // ========================================================
    // HX711 C
    // ID 90
    // ========================================================

    if (dataMasuk.nodeID == 90)
    {
        kalC = dataMasuk.value;

        pref.putFloat("C", kalC);

        Serial.println();
        Serial.println("========================");
        Serial.println("KALIBRASI C DITERIMA");
        Serial.print("C = ");
        Serial.println(kalC, 6);
        Serial.println("========================");


        balas.nodeID = 190;
        balas.value = kalC;

        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas)
        );
    }


    // ========================================================
    // TARE
    // ID 84
    // ACK 191
    //
    // TIDAK menggunakan scale.tare()
    //
    // TARE = mengambil RAW langsung
    // ========================================================

    if (dataMasuk.nodeID == 84)
    {
        Serial.println();
        Serial.println("========================");
        Serial.println("PERINTAH TARE DITERIMA");
        Serial.println("Membaca RAW TARE...");
        Serial.println("========================");


        // Baca RAW langsung
        rawT = scale.read();


        // Simpan TARE ke Preferences
        pref.putLong("TARE", rawT);


        // Reset RAW sebelumnya
        raw_sebelumnya = rawT;


        Serial.println("========================");
        Serial.println("TARE SELESAI");
        Serial.print("RAW TARE = ");
        Serial.println(rawT);
        Serial.println("Massa = 0.0 kg");
        Serial.println("========================");


        balas.nodeID = 191;
        balas.value = 1.0;


        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas)
        );
    }


    // ========================================================
    // MINTA NILAI A
    // ID 85
    // ACK 185
    // ========================================================

    if (dataMasuk.nodeID == 85)
    {
        float nilaiKalA =
            pref.getFloat("A", 1.0);


        Serial.println();
        Serial.println("========================");
        Serial.println("MINTA NILAI HX711 A");
        Serial.print("A = ");
        Serial.println(nilaiKalA, 6);
        Serial.println("========================");


        balas.nodeID = 185;
        balas.value = nilaiKalA;


        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas)
        );
    }


    // ========================================================
    // MINTA NILAI B
    // ID 86
    // ACK 186
    // ========================================================

    if (dataMasuk.nodeID == 86)
    {
        float nilaiKalB =
            pref.getFloat("B", 0.0);


        Serial.println();
        Serial.println("========================");
        Serial.println("MINTA NILAI HX711 B");
        Serial.print("B = ");
        Serial.println(nilaiKalB, 6);
        Serial.println("========================");


        balas.nodeID = 186;
        balas.value = nilaiKalB;


        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas)
        );
    }


    // ========================================================
    // MINTA NILAI C
    // ID 87
    // ACK 187
    // ========================================================

    if (dataMasuk.nodeID == 87)
    {
        float nilaiKalC =
            pref.getFloat("C", 20734.434);


        Serial.println();
        Serial.println("========================");
        Serial.println("MINTA NILAI HX711 C");
        Serial.print("C = ");
        Serial.println(nilaiKalC, 6);
        Serial.println("========================");


        balas.nodeID = 187;
        balas.value = nilaiKalC;


        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas)
        );
    }


    // ========================================================
    // HX711_R
    //
    // ID 83
    // ACK 183
    //
    // Kirim NET RAW
    // ========================================================

    if (dataMasuk.nodeID == 83)
    {
        long raw = scale.read();

        long netRaw = raw - rawT;


        Serial.println();
        Serial.println("========================");
        Serial.println("HX711_R DIMINTA");

        Serial.print("RAW     = ");
        Serial.println(raw);

        Serial.print("RAW T   = ");
        Serial.println(rawT);

        Serial.print("NET RAW = ");
        Serial.println(netRaw);

        Serial.println("========================");


        balas.nodeID = 183;

        balas.value = (float)netRaw;


        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas)
        );
    }
}


// ============================================================
// CALLBACK STATUS KIRIM
// ============================================================

void OnDataSent(const wifi_tx_info_t *info,
                esp_now_send_status_t status)
{
    Serial.print("Kirim ke: ");

    for (int i = 0; i < 6; i++)
    {
        Serial.print(info->des_addr[i], HEX);

        if (i < 5)
            Serial.print(":");
    }

    Serial.print(" | Status: ");

    Serial.println(
        status == ESP_NOW_SEND_SUCCESS
        ? "OK"
        : "FAIL"
    );
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(115200);

    pinMode(ledPin, OUTPUT);

    digitalWrite(ledPin, LOW);


    // ========================================================
    // PREFERENCES
    // ========================================================

    pref.begin("hx711", false);


    // ========================================================
    // BACA KALIBRASI
    // ========================================================

    kalC = pref.getFloat(
        "C",
        20734.434
    );

    kalA = pref.getFloat(
        "A",
        1.0
    );

    kalB = pref.getFloat(
        "B",
        0.0
    );


    // ========================================================
    // HX711 INIT
    // ========================================================

    scale.begin(
        HX711_DOUT,
        HX711_SCK
    );


    if (!scale.is_ready())
    {
        Serial.println();
        Serial.println("ERROR : HX711 TIDAK TERDETEKSI");


        while (1)
        {
            digitalWrite(
                ledPin,
                !digitalRead(ledPin)
            );

            delay(300);
        }
    }


    Serial.println();
    Serial.println("========================");
    Serial.println("HX711 OK");
    Serial.println("========================");


    delay(500);


    // ========================================================
    // BACA TARE
    // ========================================================

    if (pref.isKey("TARE"))
    {
        rawT = pref.getLong(
            "TARE",
            0
        );

        Serial.println("TARE DARI PREFERENCES");
    }
    else
    {
        // Belum ada TARE tersimpan.
        // Ambil RAW saat startup.

        rawT = scale.read();

        pref.putLong(
            "TARE",
            rawT
        );

        Serial.println("TARE BARU DIAMBIL");
    }


    raw_sebelumnya = rawT;


    // ========================================================
    // TAMPILKAN PARAMETER
    // ========================================================

    Serial.println();
    Serial.println("========================");
    Serial.println("PARAMETER HX711");

    Serial.print("RAW TARE = ");
    Serial.println(rawT);

    Serial.print("A = ");
    Serial.println(kalA, 6);

    Serial.print("B = ");
    Serial.println(kalB, 6);

    Serial.print("C = ");
    Serial.println(kalC, 6);

    Serial.println("========================");


    // ========================================================
    // WIFI
    // ========================================================

    WiFi.mode(WIFI_STA);

    WiFi.STA.begin();

    delay(100);


    // Paksa channel sama dengan gateway
    esp_wifi_set_channel(
        1,
        WIFI_SECOND_CHAN_NONE
    );


    Serial.print("MAC NODE: ");
    Serial.println(
        WiFi.macAddress()
    );


    // ========================================================
    // ESP-NOW INIT
    // ========================================================

    if (esp_now_init() != ESP_OK)
    {
        Serial.println(
            "ESP NOW ERROR"
        );

        return;
    }


    esp_now_register_send_cb(
        OnDataSent
    );

    esp_now_register_recv_cb(
        OnDataRecv
    );


    // ========================================================
    // SET PEER
    // ========================================================

    memcpy(
        peerInfo.peer_addr,
        broadcastAddress,
        6
    );

    peerInfo.channel = 1;
    peerInfo.encrypt = false;


    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println(
            "Gagal tambah peer"
        );

        return;
    }


    // ========================================================
    // SELESAI
    // ========================================================

    Serial.println();
    Serial.println("========================");
    Serial.println("SENDER LOADCELL SIAP");
    Serial.println("========================");
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
  prosesSerial();
    digitalWrite(
        ledPin,
        LOW
    );


    // ========================================================
    // INTERVAL KIRIM
    // ========================================================

    if (millis() - lastSend >= sendInterval)
    {
        lastSend = millis();


        // ====================================================
        // BACA RAW LANGSUNG
        //
        // TIDAK menggunakan get_units()
        // ====================================================

        long raw = scale.read();


        // ====================================================
        // PERUBAHAN RAW
        // ====================================================

        long perubahanRaw =
            raw - raw_sebelumnya;


        // ====================================================
        // HITUNG NET RAW
        // ====================================================

        long netRaw =
            raw - rawT;


        // ====================================================
        // HITUNG MASSA
        // ====================================================

        float kg =
            hitungKg(raw);


        // ====================================================
        // PERUBAHAN MASSA
        // ====================================================

        float perubahanKg = 0;

        if (kalC != 0)
        {
            perubahanKg =
                (float)perubahanRaw / kalC;

            // A diterapkan ke perubahan juga
            perubahanKg =
                kalA * perubahanKg;
        }


        // ====================================================
        // SERIAL
        // ====================================================

        Serial.print("RAW: ");
        Serial.print(raw);

        Serial.print(" | Perubahan RAW: ");
        Serial.print(perubahanRaw);

        Serial.print(" | NET RAW: ");
        Serial.print(netRaw);

        Serial.print(" | Massa: ");
        Serial.print(kg, 2);
        Serial.print(" kg");

        Serial.print(" | Perubahan Massa: ");
        Serial.print(perubahanKg, 2);
        Serial.print(" kg");

        Serial.print(" | TARE: ");
        Serial.print(rawT);

        Serial.print(" | A: ");
        Serial.print(kalA, 6);

        Serial.print(" | B: ");
        Serial.print(kalB, 6);

        Serial.print(" | C: ");
        Serial.println(kalC, 3);


        // ====================================================
        // ISI PACKET
        // ====================================================

        data.nodeID = 2;

        data.packetID =
            packet_counter++;

        data.value = kg;

        data.rotationID = 0;


        // ====================================================
        // KIRIM ESP-NOW
        // ====================================================

        digitalWrite(
            ledPin,
            HIGH
        );


        esp_err_t result =
            esp_now_send(
                broadcastAddress,
                (uint8_t *)&data,
                sizeof(data)
            );


        Serial.print("Berat: ");
        Serial.println(data.value, 2);


        if (result == ESP_OK)
        {
            Serial.println(
                "Queue OK"
            );
        }
        else
        {
            Serial.println(
                "Queue ERROR"
            );
        }


        // ====================================================
        // SIMPAN RAW SEKARANG
        // ====================================================

        raw_sebelumnya = raw;
    }
}
