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

/* =========================================================
   KALIBRASI HX711 #1
   ========================================================= */

float kalA_1 = -7050;   // Calibration factor
float kalB_1 = 1.0;     // Slope
float kalC_1 = 0.0;     // Offset


/* =========================================================
   KALIBRASI HX711 #2
   ========================================================= */

float kalA_2 = 52.9664; // Calibration factor
float kalB_2 = 1.0;     // Slope
float kalC_2 = 0.0;     // Offset

long offset_1 = 0;
long offset_2 = 0;

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


    

        //====================================================
    // TARE HX711 #1
    // ID 54
    //====================================================

    if (dataMasuk.nodeID == 54)
    {
        Serial.println("========================");
        Serial.println("TARE HX711 #1");
        Serial.println("========================");

        scale1.set_scale();
        scale1.tare();

        // Kembalikan calibration factor
        scale1.set_scale(kalA_1);

        Serial.println("TARE HX711 #1 SELESAI");

        balas.nodeID = 154;
        balas.value = 1.0;

        esp_now_send(broadcastAddress,
                    (uint8_t *)&balas,
                    sizeof(balas));
    } 
    //====================================================
    // TARE HX711 #2
    // ID 64
    //====================================================

    if (dataMasuk.nodeID == 64)
    {
        Serial.println("========================");
        Serial.println("TARE HX711 #2");
        Serial.println("========================");

        scale2.set_scale();
        scale2.tare();

        // Kembalikan calibration factor
        scale2.set_scale(kalA_2);

        Serial.println("TARE HX711 #2 SELESAI");

        balas.nodeID = 164;
        balas.value = 1.0;

        esp_now_send(broadcastAddress,
                    (uint8_t *)&balas,
                    sizeof(balas));
    }

    //====================================================
    // MINTA NILAI A_1
    // ID 55
    //====================================================

    if (dataMasuk.nodeID == 55)
    {
        float nilaiA_1 = pref.getFloat("A1", 0.0);

        Serial.println("========================");
        Serial.println("MINTA NILAI HX711_1 A");
        Serial.print("A_1 = ");
        Serial.println(nilaiA_1, 6);
        Serial.println("========================");

        balas.nodeID = 155;
        balas.value = nilaiA_1;

        esp_now_send(broadcastAddress,
                    (uint8_t *)&balas,
                    sizeof(balas));
    }

    //====================================================
    // MINTA NILAI B_1
    // ID 56
    //====================================================

    if (dataMasuk.nodeID == 56)
    {
        float nilaiB_1 = pref.getFloat("B1", 0.0);

        Serial.println("========================");
        Serial.println("MINTA NILAI HX711_1 B");
        Serial.print("B_1 = ");
        Serial.println(nilaiB_1, 6);
        Serial.println("========================");

        balas.nodeID = 156;
        balas.value = nilaiB_1;

        esp_now_send(broadcastAddress,
                    (uint8_t *)&balas,
                    sizeof(balas));
    }

    //====================================================
    // MINTA NILAI C_1
    // ID 57
    //====================================================

    if (dataMasuk.nodeID == 57)
    {
        float nilaiC_1 = pref.getFloat("C1", 0.0);

        Serial.println("========================");
        Serial.println("MINTA NILAI HX711_1 C");
        Serial.print("C_1 = ");
        Serial.println(nilaiC_1, 6);
        Serial.println("========================");

        balas.nodeID = 157;
        balas.value = nilaiC_1;

        esp_now_send(broadcastAddress,
                    (uint8_t *)&balas,
                    sizeof(balas));
    }

    //====================================================
    // MINTA NILAI A_2
    // ID 65
    //====================================================

    if (dataMasuk.nodeID == 65)
    {
        float nilaiA_2 = pref.getFloat("A2", 0.0);

        Serial.println("========================");
        Serial.println("MINTA NILAI HX711_2 A");
        Serial.print("A_2 = ");
        Serial.println(nilaiA_2, 6);
        Serial.println("========================");

        balas.nodeID = 165;
        balas.value = nilaiA_2;

        esp_now_send(broadcastAddress,
                    (uint8_t *)&balas,
                    sizeof(balas));
    }

    //====================================================
    // MINTA NILAI B_2
    // ID 66
    //====================================================

    if (dataMasuk.nodeID == 66)
    {
        float nilaiB_2 = pref.getFloat("B2", 0.0);

        Serial.println("========================");
        Serial.println("MINTA NILAI HX711_2 B");
        Serial.print("B_2 = ");
        Serial.println(nilaiB_2, 6);
        Serial.println("========================");

        balas.nodeID = 166;
        balas.value = nilaiB_2;

        esp_now_send(broadcastAddress,
                    (uint8_t *)&balas,
                    sizeof(balas));
    }

    //====================================================
    // MINTA NILAI C_2
    // ID 67
    //====================================================

    if (dataMasuk.nodeID == 67)
    {
        float nilaiC_2 = pref.getFloat("C2", 0.0);

        Serial.println("========================");
        Serial.println("MINTA NILAI HX711_2 C");
        Serial.print("C_2 = ");
        Serial.println(nilaiC_2, 6);
        Serial.println("========================");

        balas.nodeID = 167;
        balas.value = nilaiC_2;

        esp_now_send(broadcastAddress,
                    (uint8_t *)&balas,
                    sizeof(balas));
    }
    //====================================================
    //====================================================
    // SIMPAN NILAI A_1
    // ID 58
    //====================================================

    if (dataMasuk.nodeID == 58)
    {
        kalA_1 = dataMasuk.value;

        pref.putFloat("A1", kalA_1);

        scale1.set_scale(kalA_1);

        Serial.println("========================");
        Serial.println("SIMPAN A_1 HX711 #1");
        Serial.print("A_1 = ");
        Serial.println(kalA_1, 6);
        Serial.println("========================");

        balas.nodeID = 158;
        balas.value = kalA_1;

        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas)
        );
    }


    //====================================================
    // SIMPAN NILAI B_1
    // ID 59
    //====================================================

    if (dataMasuk.nodeID == 59)
    {
        kalB_1 = dataMasuk.value;

        pref.putFloat("B1", kalB_1);

        Serial.println("========================");
        Serial.println("SIMPAN B_1 HX711 #1");
        Serial.print("B_1 = ");
        Serial.println(kalB_1, 6);
        Serial.println("========================");

        balas.nodeID = 159;
        balas.value = kalB_1;

        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas)
        );
    }


    //====================================================
    // SIMPAN NILAI C_1
    // ID 60
    //====================================================

    if (dataMasuk.nodeID == 60)
    {
        kalC_1 = dataMasuk.value;

        pref.putFloat("C1", kalC_1);

        Serial.println("========================");
        Serial.println("SIMPAN C_1 HX711 #1");
        Serial.print("C_1 = ");
        Serial.println(kalC_1, 6);
        Serial.println("========================");

        balas.nodeID = 160;
        balas.value = kalC_1;

        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas)
        );
    }


    //====================================================
    // SIMPAN NILAI A_2
    // ID 68
    //====================================================

    if (dataMasuk.nodeID == 68)
    {
        kalA_2 = dataMasuk.value;

        pref.putFloat("A2", kalA_2);

        scale2.set_scale(kalA_2);

        Serial.println("========================");
        Serial.println("SIMPAN A_2 HX711 #2");
        Serial.print("A_2 = ");
        Serial.println(kalA_2, 6);
        Serial.println("========================");

        balas.nodeID = 168;
        balas.value = kalA_2;

        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas)
        );
    }


    //====================================================
    // SIMPAN NILAI B_2
    // ID 69
    //====================================================

    if (dataMasuk.nodeID == 69)
    {
        kalB_2 = dataMasuk.value;

        pref.putFloat("B2", kalB_2);

        Serial.println("========================");
        Serial.println("SIMPAN B_2 HX711 #2");
        Serial.print("B_2 = ");
        Serial.println(kalB_2, 6);
        Serial.println("========================");

        balas.nodeID = 169;
        balas.value = kalB_2;

        esp_now_send(
            broadcastAddress,
            (uint8_t *)&balas,
            sizeof(balas)
        );
    }


    //====================================================
    // SIMPAN NILAI C_2
    // ID 70
    //====================================================

    if (dataMasuk.nodeID == 70)
    {
        kalC_2 = dataMasuk.value;

        pref.putFloat("C2", kalC_2);

        Serial.println("========================");
        Serial.println("SIMPAN C_2 HX711 #2");
        Serial.print("C_2 = ");
        Serial.println(kalC_2, 6);
        Serial.println("========================");

        balas.nodeID = 170;
        balas.value = kalC_2;

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
    pref.begin("HX711", false);
    digitalWrite(ledPin, LOW);


  //====================================================
// BACA KALIBRASI HX711 #1
//====================================================

kalA_1 = pref.getFloat("A1", -7050);
kalB_1 = pref.getFloat("B1", 1.0);
kalC_1 = pref.getFloat("C1", 0.0);

//====================================================
// BACA KALIBRASI HX711 #2
//====================================================

kalA_2 = pref.getFloat("A2", 52.9664);
kalB_2 = pref.getFloat("B2", 1.0);
kalC_2 = pref.getFloat("C2", 0.0);

//====================================================
// TAMPILKAN HASIL
//====================================================

Serial.println("========== EEPROM ==========");

Serial.print("A1 = ");
Serial.println(kalA_1, 6);

Serial.print("B1 = ");
Serial.println(kalB_1, 6);

Serial.print("C1 = ");
Serial.println(kalC_1, 6);

Serial.print("A2 = ");
Serial.println(kalA_2, 6);

Serial.print("B2 = ");
Serial.println(kalB_2, 6);

Serial.print("C2 = ");
Serial.println(kalC_2, 6);

Serial.println("============================");


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

    scale1.set_scale(kalA_1);
    scale2.set_scale(kalA_2);


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
   KALIBRASI HX711 #1
   ================================================= */

    float hasil1 =kalB_1 * nilai1 + kalC_1;

    /* =================================================
    KALIBRASI HX711 #2
    ================================================= */

    float hasil2 =kalB_2 * nilai2 + kalC_2;
  /* =================================================
   JUMLAH HASIL KEDUA HX711
   ================================================= */
    float total =(hasil1 + hasil2) * -1;
    data.value = total;

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


        Serial.print("HASIL HX711 #1 : ");
        Serial.println(hasil1);

        Serial.print("HASIL HX711 #2 : " );
        Serial.println(hasil2);


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