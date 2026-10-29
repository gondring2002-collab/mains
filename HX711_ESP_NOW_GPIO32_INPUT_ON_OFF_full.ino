#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "HX711.h"
#include <Preferences.h>

// ============================================================
// HX711 CONFIG
// ============================================================
#define HX711_DOUT 27
#define HX711_SCK  26
#define PIN_MODE_RAW 32

HX711 scale;

// ============================================================
// KALIBRASI
// ============================================================
float kalC = 19910.301f;
long rawT = 0;
long rawK = 0;
bool rawK_valid = false;

const float MASSA_KALIBRASI = 17.96f;

const uint8_t SAMPEL_TARE = 15;
const uint8_t SAMPEL_KAL  = 15;
const uint8_t SAMPEL_LOOP = 5;

Preferences pref;

// ============================================================
// MODE ON/OFF
// ON menyimpan massa aktual sebagai referensi sementara.
// OFF mengembalikan pengiriman massa aktual terhadap tare.
// ============================================================
bool modeDeltaRAW = false;
float massaReferensiON = 0.0f;
bool statusSakelarSebelumnya = HIGH;

// RAW sebelumnya untuk informasi dRAW saja
long raw_sebelumnya = 0;

// ============================================================
// DATA ESP-NOW
// ============================================================
typedef struct __attribute__((packed))
{
    uint8_t  nodeID;
    uint32_t packetID;
    float    value;
    uint8_t  rotationID;
} sensorPacket;

sensorPacket data;
uint32_t packet_counter = 0;

const int ledPin = 2;
uint32_t sendInterval = 700;
uint32_t lastSend = 0;

uint8_t gatewayAddress[] = {0x88, 0x57, 0x21, 0x2E, 0x6E, 0x4C};
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t peerInfo;

uint32_t txOK = 0;
uint32_t txFail = 0;
uint32_t rxCount = 0;
uint32_t lastStatusPrint = 0;

// ============================================================
// FUNGSI BANTU
// ============================================================
long bacaRataRata(uint8_t n)
{
    return (long)scale.read_average(n);
}

float hitungKg(long raw)
{
    if (kalC == 0.0f) return 0.0f;
    return (float)(raw - rawT) / kalC;
}

void printMAC(const uint8_t *mac)
{
    for (int i = 0; i < 6; i++)
    {
        if (mac[i] < 16) Serial.print("0");
        Serial.print(mac[i], HEX);
        if (i < 5) Serial.print(":");
    }
}

// ============================================================
// FUNGSI MODE ON/OFF BERSAMA
// Dipakai oleh Serial ON/OFF dan sakelar fisik GPIO32.
// ============================================================
void aktifkanModeON(long rawReferensi)
{
    massaReferensiON = hitungKg(rawReferensi);
    modeDeltaRAW = true;
    raw_sebelumnya = rawReferensi;

    Serial.println();
    Serial.println("========== MODE ON ==========");
    Serial.print("RAW referensi = ");
    Serial.println(rawReferensi);
    Serial.print("Massa referensi tersimpan = ");
    Serial.print(massaReferensiON, 3);
    Serial.println(" kg");
    Serial.println("Nilai kirim = massa aktual - massa referensi");
    Serial.println("=============================");
}

void aktifkanModeON()
{
    aktifkanModeON(bacaRataRata(SAMPEL_LOOP));
}

void nonaktifkanModeON()
{
    modeDeltaRAW = false;
    Serial.println();
    Serial.println("========== MODE OFF =========");
    Serial.println("Pengiriman kembali ke massa aktual terhadap tare.");
    Serial.println("==============================");
}

// ============================================================
// SERIAL COMMAND: ON / OFF / T / K / C / g=nilai / S / R
// ============================================================
void prosesSerial()
{
    if (!Serial.available()) return;

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.length() == 0) return;

    // ON: sama persis dengan sakelar GPIO32 posisi ON.
    if (cmd.equalsIgnoreCase("ON"))
    {
        aktifkanModeON();
        return;
    }

    // OFF: sama persis dengan sakelar GPIO32 posisi OFF.
    if (cmd.equalsIgnoreCase("OFF"))
    {
        nonaktifkanModeON();
        return;
    }

    char c = cmd.charAt(0);

    // T = tare
    if (c == 'T' || c == 't')
    {
        rawT = bacaRataRata(SAMPEL_TARE);
        raw_sebelumnya = rawT;
        pref.putLong("TARE", rawT);

        Serial.println();
        Serial.println("========== TARE ==========");
        Serial.print("RAW T = ");
        Serial.println(rawT);
        Serial.println("Massa = 0.0 kg");
        Serial.println("==========================");
    }

    // K = baca RAW dengan beban kalibrasi
    else if (c == 'K' || c == 'k')
    {
        rawK = bacaRataRata(SAMPEL_KAL);
        rawK_valid = true;

        Serial.println();
        Serial.println("======= BEBAN KALIBRASI =======");
        Serial.print("RAW K = ");
        Serial.println(rawK);
        Serial.print("Massa diketahui = ");
        Serial.print(MASSA_KALIBRASI, 2);
        Serial.println(" kg");
        Serial.println("===============================");
    }

    // C = hitung calibration factor
    else if (c == 'C' || c == 'c')
    {
        if (!rawK_valid)
        {
            Serial.println("ERROR: Tekan K terlebih dahulu.");
        }
        else
        {
            long perubahanRaw = rawK - rawT;

            if (perubahanRaw == 0 || MASSA_KALIBRASI <= 0)
            {
                Serial.println("ERROR: Data kalibrasi tidak valid.");
            }
            else
            {
                kalC = (float)perubahanRaw / MASSA_KALIBRASI;
                pref.putFloat("C", kalC);

                Serial.println();
                Serial.println("====== HASIL KALIBRASI ======");
                Serial.print("RAW T         = ");
                Serial.println(rawT);
                Serial.print("RAW K         = ");
                Serial.println(rawK);
                Serial.print("Perubahan RAW = ");
                Serial.println(perubahanRaw);
                Serial.print("Massa         = ");
                Serial.print(MASSA_KALIBRASI, 2);
                Serial.println(" kg");
                Serial.print("CAL FACTOR    = ");
                Serial.println(kalC, 3);
                Serial.println("CF tersimpan.");
                Serial.println("=============================");
            }
        }
    }

    // g=10000 : masukkan CF secara langsung
    else if (c == 'G' || c == 'g')
    {
        int posisiSama = cmd.indexOf('=');

        if (posisiSama >= 0)
        {
            float cfBaru = cmd.substring(posisiSama + 1).toFloat();

            if (isfinite(cfBaru) && cfBaru > 0.0f)
            {
                kalC = cfBaru;
                pref.putFloat("C", kalC);

                Serial.print("CF tersimpan = ");
                Serial.println(kalC, 3);
            }
            else
            {
                Serial.println("ERROR: CF harus lebih besar dari 0.");
            }
        }
        else
        {
            Serial.println("Format salah. Contoh: g=10000");
        }
    }

    // S = simpan CF saat ini
    else if (c == 'S' || c == 's')
    {
        pref.putFloat("C", kalC);
        Serial.print("CF disimpan = ");
        Serial.println(kalC, 3);
    }

    // R = reset CF ke nilai default lama
    else if (c == 'R' || c == 'r')
    {
        kalC = 20734.434f;
        pref.putFloat("C", kalC);

        Serial.print("CF direset ke default = ");
        Serial.println(kalC, 3);
    }
}

// ============================================================
// CALLBACK ESP-NOW RECEIVE — kompatibel core 2.x dan 3.x
// ============================================================
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *incomingData,
                int len)
{
    const uint8_t *mac = info->src_addr;
#else
void OnDataRecv(const uint8_t *mac,
                const uint8_t *incomingData,
                int len)
{
#endif
    rxCount++;

    Serial.println();
    Serial.println("================================");
    Serial.print("[RX] Dari: ");
    printMAC(mac);
    Serial.print(" | Len: ");
    Serial.println(len);

    if (len != sizeof(sensorPacket))
    {
        Serial.println("[RX] Ukuran paket tidak cocok, diabaikan.");
        Serial.println("================================");
        return;
    }

    sensorPacket dataMasuk;
    memcpy(&dataMasuk, incomingData, sizeof(sensorPacket));

    Serial.print("[RX] nodeID = ");
    Serial.println(dataMasuk.nodeID);
    Serial.print("[RX] packetID = ");
    Serial.println(dataMasuk.packetID);
    Serial.print("[RX] value = ");
    Serial.println(dataMasuk.value, 6);
    Serial.print("[RX] rotationID = ");
    Serial.println(dataMasuk.rotationID);
    Serial.println("================================");

    sensorPacket balas = {};
    balas.packetID = 0;
    balas.rotationID = 0;

    if (dataMasuk.nodeID == 88)
    {
        Serial.println("[CMD] KALIBRASI A DITERIMA (diabaikan, A=1.0)");
        balas.nodeID = 188;
        balas.value = 1.0f;
        esp_err_t r = esp_now_send(gatewayAddress, (uint8_t *)&balas, sizeof(balas));
        Serial.print("[TX-ACK188] ");
        Serial.println(r == ESP_OK ? "Queued" : "ERROR");
    }

    if (dataMasuk.nodeID == 89)
    {
        Serial.println("[CMD] KALIBRASI B DITERIMA (diabaikan, B=0.0)");
        balas.nodeID = 189;
        balas.value = 0.0f;
        esp_err_t r = esp_now_send(gatewayAddress, (uint8_t *)&balas, sizeof(balas));
        Serial.print("[TX-ACK189] ");
        Serial.println(r == ESP_OK ? "Queued" : "ERROR");
    }

    if (dataMasuk.nodeID == 90)
    {
        kalC = dataMasuk.value;
        pref.putFloat("C", kalC);

        Serial.print("[CMD] KALIBRASI C DITERIMA -> C = ");
        Serial.println(kalC, 6);

        balas.nodeID = 190;
        balas.value = kalC;
        esp_err_t r = esp_now_send(gatewayAddress, (uint8_t *)&balas, sizeof(balas));
        Serial.print("[TX-ACK190] ");
        Serial.println(r == ESP_OK ? "Queued" : "ERROR");
    }

    if (dataMasuk.nodeID == 84)
    {
        Serial.println("[CMD] PERINTAH TARE DITERIMA");
        rawT = bacaRataRata(SAMPEL_TARE);
        pref.putLong("TARE", rawT);
        raw_sebelumnya = rawT;

        Serial.print("[CMD] TARE SELESAI, RAW TARE = ");
        Serial.println(rawT);

        balas.nodeID = 191;
        balas.value = 1.0f;
        esp_err_t r = esp_now_send(gatewayAddress, (uint8_t *)&balas, sizeof(balas));
        Serial.print("[TX-ACK191] ");
        Serial.println(r == ESP_OK ? "Queued" : "ERROR");
    }

    if (dataMasuk.nodeID == 85)
    {
        Serial.println("[CMD] MINTA NILAI A -> 1.0");
        balas.nodeID = 185;
        balas.value = 1.0f;
        esp_err_t r = esp_now_send(gatewayAddress, (uint8_t *)&balas, sizeof(balas));
        Serial.print("[TX-ACK185] ");
        Serial.println(r == ESP_OK ? "Queued" : "ERROR");
    }

    if (dataMasuk.nodeID == 86)
    {
        Serial.println("[CMD] MINTA NILAI B -> 0.0");
        balas.nodeID = 186;
        balas.value = 0.0f;
        esp_err_t r = esp_now_send(gatewayAddress, (uint8_t *)&balas, sizeof(balas));
        Serial.print("[TX-ACK186] ");
        Serial.println(r == ESP_OK ? "Queued" : "ERROR");
    }

    if (dataMasuk.nodeID == 87)
    {
        float nilaiKalC = pref.getFloat("C", 20734.434f);
        Serial.print("[CMD] MINTA NILAI C -> ");
        Serial.println(nilaiKalC, 6);

        balas.nodeID = 187;
        balas.value = nilaiKalC;
        esp_err_t r = esp_now_send(gatewayAddress, (uint8_t *)&balas, sizeof(balas));
        Serial.print("[TX-ACK187] ");
        Serial.println(r == ESP_OK ? "Queued" : "ERROR");
    }

    if (dataMasuk.nodeID == 83)
    {
        long raw = bacaRataRata(SAMPEL_LOOP);
        long netRaw = raw - rawT;

        Serial.print("[CMD] HX711_R DIMINTA, NET RAW = ");
        Serial.println(netRaw);

        balas.nodeID = 183;
        balas.value = (float)netRaw;
        esp_err_t r = esp_now_send(gatewayAddress, (uint8_t *)&balas, sizeof(balas));
        Serial.print("[TX-ACK183] ");
        Serial.println(r == ESP_OK ? "Queued" : "ERROR");
    }
}

// ============================================================
// CALLBACK STATUS KIRIM — kompatibel core 2.x dan 3.x
// ============================================================
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataSent(const wifi_tx_info_t *info,
                esp_now_send_status_t status)
{
    const uint8_t *mac = info->des_addr;
#else
void OnDataSent(const uint8_t *mac,
                esp_now_send_status_t status)
{
#endif
    if (status == ESP_NOW_SEND_SUCCESS) txOK++;
    else txFail++;

    Serial.print("[TX] Ke: ");
    printMAC(mac);
    Serial.print(" | Status: ");
    Serial.print(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
    Serial.print(" | Total OK=");
    Serial.print(txOK);
    Serial.print(" FAIL=");
    Serial.println(txFail);
}

// ============================================================
// SETUP
// ============================================================
void setup()
{
    Serial.begin(115200);
    delay(300);
    Serial.setTimeout(100);

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    // GPIO32 sebagai INPUT_PULLUP.
    // Sakelar: satu kaki ke GPIO32, kaki lain ke GND.
    // Sakelar tertutup (LOW) = ON; terbuka (HIGH) = OFF.
    pinMode(PIN_MODE_RAW, INPUT_PULLUP);
    statusSakelarSebelumnya = HIGH;
    modeDeltaRAW = false;

    // Preferences
    pref.begin("hx711", false);
    kalC = pref.getFloat("C", 27816.22f);

    // HX711 init
    scale.begin(HX711_DOUT, HX711_SCK);

    if (!scale.is_ready())
    {
        Serial.println();
        Serial.println("ERROR: HX711 TIDAK TERDETEKSI");
        while (1)
        {
            digitalWrite(ledPin, !digitalRead(ledPin));
            delay(300);
        }
    }

    Serial.println();
    Serial.println("========================");
    Serial.println("HX711 OK");
    Serial.println("========================");

    delay(2000);

    // Tare startup
    rawT = bacaRataRata(SAMPEL_TARE);
    raw_sebelumnya = rawT;
    pref.putLong("TARE", rawT);

    Serial.println();
    Serial.println("========================");
    Serial.println("TARE STARTUP");
    Serial.println("========================");
    Serial.print("RAW TARE = ");
    Serial.println(rawT);
    Serial.println("Massa = 0.0 kg");
    Serial.println("========================");

    Serial.println();
    Serial.println("========================");
    Serial.println("PARAMETER HX711");
    Serial.print("RAW TARE = ");
    Serial.println(rawT);
    Serial.print("C        = ");
    Serial.println(kalC, 6);
    Serial.println("========================");

    // WiFi
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    Serial.print("MAC NODE: ");
    Serial.println(WiFi.macAddress());

    // ESP-NOW init
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ESP NOW INIT GAGAL");
        return;
    }

    Serial.println("ESP NOW INIT OK");
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    // Gateway peer
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, gatewayAddress, 6);
    peerInfo.channel = 1;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Gagal tambah peer GATEWAY");
        return;
    }

    Serial.print("Peer GATEWAY ditambahkan: ");
    printMAC(gatewayAddress);
    Serial.println();

    // Broadcast peer (opsional)
    esp_now_peer_info_t peerBcast = {};
    memcpy(peerBcast.peer_addr, broadcastAddress, 6);
    peerBcast.channel = 1;
    peerBcast.encrypt = false;

    if (esp_now_add_peer(&peerBcast) != ESP_OK)
    {
        Serial.println("(Broadcast peer gagal ditambah, lanjut saja)");
    }
    else
    {
        Serial.println("Peer BROADCAST ditambahkan");
    }

    Serial.println();
    Serial.println("========================");
    Serial.println("SENDER LOADCELL SIAP");
    Serial.println("Serial ON/OFF atau sakelar GPIO32 mengubah mode.");
    Serial.println("GPIO32 INPUT_PULLUP: hubungkan sakelar antara GPIO32 dan GND.");
    Serial.println("Sakelar ON (LOW) menyimpan massa referensi.");
    Serial.println("Sakelar OFF (HIGH) kembali ke massa aktual.");
    Serial.println("========================");
}

// ============================================================
// LOOP
// ============================================================
void loop()
{
    prosesSerial();
    digitalWrite(ledPin, LOW);

    // GPIO32 menjalankan fungsi yang sama dengan Serial ON/OFF.
    bool statusSakelar = digitalRead(PIN_MODE_RAW);

    // Perubahan HIGH -> LOW: sakelar baru diaktifkan (ON).
    if (statusSakelarSebelumnya == HIGH && statusSakelar == LOW)
    {
        Serial.println("[GPIO32] Sakelar ON terdeteksi");
        aktifkanModeON();
    }
    // Perubahan LOW -> HIGH: sakelar dinonaktifkan (OFF).
    else if (statusSakelarSebelumnya == LOW && statusSakelar == HIGH)
    {
        Serial.println("[GPIO32] Sakelar OFF terdeteksi");
        nonaktifkanModeON();
    }

    statusSakelarSebelumnya = statusSakelar;

    if (millis() - lastSend >= sendInterval)
    {
        lastSend = millis();

        long raw = bacaRataRata(SAMPEL_LOOP);
        long perubahanRaw = raw - raw_sebelumnya;
        long netRaw = raw - rawT;

        float massaAktual = hitungKg(raw);
        float nilaiKirim;

        if (modeDeltaRAW)
        {
            nilaiKirim = massaAktual - massaReferensiON;
        }
        else
        {
            nilaiKirim = massaAktual;
        }

        Serial.print("RAW: ");
        Serial.print(raw);
        Serial.print(" | dRAW: ");
        Serial.print(perubahanRaw);
        Serial.print(" | NET RAW: ");
        Serial.print(netRaw);
        Serial.print(" | Massa aktual: ");
        Serial.print(massaAktual, 3);
        Serial.print(" kg | Referensi ON: ");
        Serial.print(massaReferensiON, 3);
        Serial.print(" kg | Mode: ");
        Serial.print(modeDeltaRAW ? "ON-SELISIH" : "OFF-NORMAL");
        Serial.print(" | Massa dikirim: ");
        Serial.print(nilaiKirim, 3);
        Serial.println(" kg");

        raw_sebelumnya = raw;

        // Isi paket ESP-NOW
        data.nodeID = 2;
        data.packetID = packet_counter++;
        data.value = nilaiKirim;
        data.rotationID = 0;

        // Kirim ESP-NOW
        digitalWrite(ledPin, HIGH);

        esp_err_t result = esp_now_send(
            gatewayAddress,
            (uint8_t *)&data,
            sizeof(data)
        );

        Serial.print("Berat dikirim: ");
        Serial.print(data.value, 3);
        Serial.print(" kg | packetID=");
        Serial.print(data.packetID);
        Serial.print(" | ");
        Serial.println(result == ESP_OK ? "Queue OK" : "Queue ERROR");
    }

    // Status setiap 10 detik
    if (millis() - lastStatusPrint >= 10000)
    {
        lastStatusPrint = millis();

        Serial.println();
        Serial.println("========== STATUS ESP-NOW ==========");
        Serial.print("TX OK   : ");
        Serial.println(txOK);
        Serial.print("TX FAIL : ");
        Serial.println(txFail);
        Serial.print("RX TOTAL: ");
        Serial.println(rxCount);
        Serial.println("====================================");
        Serial.println();
    }
}
