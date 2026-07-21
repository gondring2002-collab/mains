#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "HX711.h"
#include <Preferences.h>


/* =========================================================
   HX711 CONFIGURATION
   ========================================================= */

// ==========================
// HX711 #1
// ==========================
#define HX711_DOUT_1 15
#define HX711_SCK_1  4

// ==========================
// HX711 #2
// ==========================
#define HX711_DOUT_2 19
#define HX711_SCK_2  18


HX711 scale1;
HX711 scale2;


/* =========================================================
   KALIBRASI
   ========================================================= */

// Kalibrasi HX711 #1
float kalA = -7050;

// Kalibrasi HX711 #2
float kalD =  52.9664;//1.0;

// Slope
float kalB = 1.0;

// Offset
float kalC = 0.0;


Preferences pref;


/* =========================================================
   DATA STRUCTURE
   ========================================================= */

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


/* =========================================================
   LED
   ========================================================= */

const int ledPin = 2;


/* =========================================================
   SEND INTERVAL
   ========================================================= */

uint32_t sendInterval = 700;
uint32_t lastSend = 0;


/* =========================================================
   MAC GATEWAY
   ========================================================= */

uint8_t broadcastAddress[] =
{
    0x88,
    0x57,
    0x21,
    0x2E,
    0x6E,
    0x4C
};


esp_now_peer_info_t peerInfo;


/* =========================================================
   CALLBACK DATA MASUK
   ========================================================= */

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


    // =====================================================
    // KALIBRASI HX711 #1
    // nodeID 88
    // =====================================================

    if (dataMasuk.nodeID == 88)
    {
        kalA = dataMasuk.value;

        pref.putFloat("A", kalA);

        scale1.set_scale(kalA);


        Serial.println("========================");
        Serial.println("Kalibrasi HX711 #1 diterima");

        Serial.print("Kalibrasi A = ");
        Serial.println(kalA);

        Serial.println("========================");


        balas.nodeID = 188;
        balas.value = kalA;


        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas)
        );
    }


    // =====================================================
    // KALIBRASI HX711 #2
    // nodeID 89
    // =====================================================

    if (dataMasuk.nodeID == 89)
    {
        kalD = dataMasuk.value;

        pref.putFloat("D", kalD);

        scale2.set_scale(kalD);


        Serial.println("========================");
        Serial.println("Kalibrasi HX711 #2 diterima");

        Serial.print("Kalibrasi D = ");
        Serial.println(kalD);

        Serial.println("========================");


        balas.nodeID = 189;
        balas.value = kalD;


        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas)
        );
    }


    // =====================================================
    // SLOPE
    // nodeID 90
    // =====================================================

    if (dataMasuk.nodeID == 90)
    {
        kalB = dataMasuk.value;

        pref.putFloat("B", kalB);


        Serial.println("========================");
        Serial.println("Kalibrasi B diterima");

        Serial.print("B = ");
        Serial.println(kalB);

        Serial.println("========================");


        balas.nodeID = 190;
        balas.value = kalB;


        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas)
        );
    }


    // =====================================================
    // OFFSET
    // nodeID 91
    // =====================================================

    if (dataMasuk.nodeID == 91)
    {
        kalC = dataMasuk.value;

        pref.putFloat("C", kalC);


        Serial.println("========================");
        Serial.println("Kalibrasi C diterima");

        Serial.print("C = ");
        Serial.println(kalC);

        Serial.println("========================");


        balas.nodeID = 191;
        balas.value = kalC;


        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas)
        );
    }
}


/* =========================================================
   CALLBACK STATUS KIRIM
   ========================================================= */

void OnDataSent(
    const wifi_tx_info_t *info,
    esp_now_send_status_t status
)
{
    Serial.print("Kirim ke: ");


    for (int i = 0; i < 6; i++)
    {
        Serial.print(
            info->des_addr[i],
            HEX
        );

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

/* =========================================================
   READ LOAD CELL #1
   ========================================================= */

float readLoadCell1()
{
    float total = 0;
    int valid = 0;


    for (int i = 0; i < 10; i++)
    {
        float val = scale1.get_units(1);


        // Filter nilai aneh
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
        Serial.println("DATA ERROR HX711 #1");
        return 0;
    }


    return total / valid;
}


/* =========================================================
   READ LOAD CELL #2
   ========================================================= */

float readLoadCell2()
{
    float total = 0;
    int valid = 0;


    for (int i = 0; i < 10; i++)
    {
        float val = scale2.get_units(1);


        // Filter nilai aneh
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
        Serial.println("DATA ERROR HX711 #2");
        return 0;
    }


    return total / valid;
}


/* =========================================================
   SETUP
   ========================================================= */

void setup()
{
    Serial.begin(115200);


    pinMode(ledPin, OUTPUT);

    digitalWrite(ledPin, LOW);


    /* =====================================================
       PREFERENCES
       ===================================================== */

    pref.begin(
        "hx711",
        false
    );


    kalA = pref.getFloat(
        "A",
        -7050
    );


    kalD = pref.getFloat(
        "D",
        52.9664
    );


    kalB = pref.getFloat(
        "B",
        1.0
    );


    kalC = pref.getFloat(
        "C",
        0.0
    );


    Serial.println("========================");

    Serial.println("Kalibrasi EEPROM :");

    Serial.print("HX711 #1 A = ");
    Serial.println(kalA);

    Serial.print("HX711 #2 D = ");
    Serial.println(kalD);

    Serial.print("Slope B = ");
    Serial.println(kalB);

    Serial.print("Offset C = ");
    Serial.println(kalC);

    Serial.println("========================");


    /* =====================================================
       HX711 #1 INIT
       ===================================================== */

    scale1.begin(
        HX711_DOUT_1,
        HX711_SCK_1
    );
    /*

    if (!scale1.is_ready())
    {
        Serial.println(
            "ERROR : HX711 #1 TIDAK TERDETEKSI"
        );


        while (1)
        {
            digitalWrite(
                ledPin,
                !digitalRead(ledPin)
            );

            delay(300);
        }
    }

   */
    Serial.println("HX711 #1 OK");
    

    /* =====================================================
       HX711 #2 INIT
       ===================================================== */

    scale2.begin(
        HX711_DOUT_2,
        HX711_SCK_2
    );
    /*

    if (!scale2.is_ready())
    {
        Serial.println(
            "ERROR : HX711 #2 TIDAK TERDETEKSI"
        );


        while (1)
        {
            digitalWrite(
                ledPin,
                !digitalRead(ledPin)
            );

            delay(300);
        }
    }

    */
    Serial.println("HX711 #2 OK");


    delay(500);


    /* =====================================================
       SET KALIBRASI
       ===================================================== */

    scale1.set_scale(kalA);

    scale2.set_scale(kalD);


    /* =====================================================
       TARE HX711 #1
       ===================================================== */

    scale1.tare();

    Serial.println(
        "Tare HX711 #1 done"
    );


    /* =====================================================
       TARE HX711 #2
       ===================================================== */

    scale2.tare();

    Serial.println(
        "Tare HX711 #2 done"
    );


    /* =====================================================
       WIFI INIT
       ===================================================== */

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


    /* =====================================================
       ESP-NOW INIT
       ===================================================== */

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


    /* =====================================================
       SET PEER
       ===================================================== */

    memcpy(
        peerInfo.peer_addr,
        broadcastAddress,
        6
    );


    peerInfo.channel = 1;

    peerInfo.encrypt = false;


    if (
        esp_now_add_peer(
            &peerInfo
        ) != ESP_OK
    )
    {
        Serial.println(
            "Gagal tambah peer"
        );

        return;
    }


    Serial.println(
        "DUAL HX711 ESP-NOW SENDER SIAP"
    );
}


/* =========================================================
   LOOP
   ========================================================= */

void loop()
{
    digitalWrite(
        ledPin,
        LOW
    );


    if (
        millis() - lastSend >= sendInterval
    )
    {
        lastSend = millis();


        /* =================================================
           BACA HX711 #1
           ================================================= */

        float nilai1 =
            readLoadCell1();


        /* =================================================
           BACA HX711 #2
           ================================================= */

        float nilai2 =
            readLoadCell2();


        /* =================================================
           JUMLAHKAN KEDUA HX711
           ================================================= */

        float raw =
            (nilai1 + nilai2) * -1;


        /* =================================================
           KALIBRASI TAMBAHAN
           ================================================= */

        data.value =
            kalB * raw + kalC;


        data.nodeID = 2;

        data.packetID =
            packet_counter++;


        data.rotationID = 0;


        /* =================================================
           KIRIM ESP-NOW
           ================================================= */

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


        Serial.print(
            "HX711 #1 : "
        );

        Serial.println(
            nilai1
        );


        Serial.print(
            "HX711 #2 : "
        );

        Serial.println(
            nilai2
        );


        Serial.print(
            "TOTAL : "
        );

        Serial.println(
            nilai1 + nilai2
        );


        Serial.print(
            "Berat dikirim : "
        );

        Serial.println(
            data.value
        );


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
    }
}