#include <Arduino.h>
#include "HX711.h"

const int LOADCELL_DOUT_PIN = 27;//13;
const int LOADCELL_SCK_PIN  = 26;//12;

HX711 scale;

// =========================
// VARIABEL
// =========================
long raw_sebelumnya = 0;

// TARE
long rawT = 0;

// RAW BEBAN 10 KG
long rawK = 0;

// CALIBRATION FACTOR
float calibrationFactor = 20734.434;

// BEBAN KALIBRASI
const float MASSA_KALIBRASI = 10.0;


// =========================
// HITUNG MASSA
// =========================
float hitungKg(long raw)
{
  return (float)(raw - rawT) / calibrationFactor;
}


void setup()
{
  Serial.begin(115200);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  Serial.println("================================");
  Serial.println("       HX711 CALIBRATION");
  Serial.println("================================");

  Serial.println("T/t = TARE");
  Serial.println("K/k = Ambil RAW beban 10 kg");
  Serial.println("C/c = Hitung Calibration Factor");
  Serial.println();

  // =========================
  // TARE AWAL
  // =========================
  rawT = scale.read();

  raw_sebelumnya = rawT;

  Serial.print("RAW TARE AWAL = ");
  Serial.println(rawT);

  Serial.print("CF AWAL = ");
  Serial.println(calibrationFactor, 3);

  Serial.println();
}


void loop()
{
  // ==================================================
  // PERINTAH SERIAL
  // ==================================================
  if (Serial.available())
  {
    char cmd = Serial.read();


    // ==================================================
    // TARE
    // ==================================================
    if (cmd == 'T' || cmd == 't')
    {
      rawT = scale.read();

      raw_sebelumnya = rawT;

      Serial.println();
      Serial.println("========== TARE ==========");

      Serial.print("RAW T = ");
      Serial.println(rawT);

      Serial.println("Massa = 0.0 kg");

      Serial.println("==========================");
      Serial.println();
    }


    // ==================================================
    // K = AMBIL RAW BEBAN 10 KG
    // ==================================================
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


    // ==================================================
    // C = HITUNG CALIBRATION FACTOR
    // ==================================================
    else if (cmd == 'C' || cmd == 'c')
    {
      long perubahanRaw = rawK - rawT;

      // Hitung CF baru
      calibrationFactor =
        (float)perubahanRaw / MASSA_KALIBRASI;

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

      Serial.print("Perubahan Massa = ");
      Serial.print(MASSA_KALIBRASI, 1);
      Serial.println(" kg");

      Serial.print("CALIBRATION FACTOR = ");
      Serial.println(calibrationFactor, 3);

      Serial.println("=============================");
      Serial.println();
    }
  }


  // ==================================================
  // BACA RAW
  // ==================================================
  long raw = scale.read();


  // ==================================================
  // PERUBAHAN RAW
  // ==================================================
  long perubahanRaw = raw - raw_sebelumnya;


  // ==================================================
  // MASSA
  // ==================================================
  float kg = hitungKg(raw);


  // ==================================================
  // PERUBAHAN MASSA
  // ==================================================
  float perubahanKg =
    (float)perubahanRaw / calibrationFactor;


  // ==================================================
  // SERIAL LOOP
  // ==================================================
  Serial.print("RAW: ");
  Serial.print(raw);

  Serial.print(" | Perubahan RAW: ");
  Serial.print(perubahanRaw);

  Serial.print(" | Massa: ");
  Serial.print(kg, 1);
  Serial.print(" kg");

  Serial.print(" | Perubahan Massa: ");
  Serial.print(perubahanKg, 1);
  Serial.print(" kg");

  // =========================
  // TARE DAN CF DI LOOP
  // =========================
  Serial.print(" | TARE: ");
  Serial.print(rawT);

  Serial.print(" | CF: ");
  Serial.println(calibrationFactor, 3);


  // Simpan RAW sekarang
  raw_sebelumnya = raw;

  delay(1000);
}