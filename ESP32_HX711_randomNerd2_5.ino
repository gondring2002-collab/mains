#include <Arduino.h>
#include "HX711.h"

// ==================================================
// PIN HX711 #1
// ==================================================
const int DOUT1 = 27;
const int SCK1  = 26;

// ==================================================
// PIN HX711 #2
// ==================================================
const int DOUT2 = 13;
const int SCK2  = 12;

// ==================================================
HX711 scale1;
HX711 scale2;

// ==================================================
// VARIABEL HX711 #1
// ==================================================
long raw_sebelumnya1 = 0;
long rawT1 = 0;
long rawK1 = 0;

float calibrationFactor1 = 48111.898;

// ==================================================
// VARIABEL HX711 #2
// ==================================================
long raw_sebelumnya2 = 0;
long rawT2 = 0;
long rawK2 = 0;

float calibrationFactor2 = 60901.602;

// ==================================================
// BEBAN KALIBRASI
// ==================================================
const float MASSA_KALIBRASI = 10.0;


// ==================================================
// HITUNG MASSA
// ==================================================
float hitungKg1(long raw)
{
  return (float)(raw - rawT1) / calibrationFactor1;
}

float hitungKg2(long raw)
{
  return (float)(raw - rawT2) / calibrationFactor2;
}


// ==================================================
// SETUP
// ==================================================
void setup()
{
  Serial.begin(115200);

  scale1.begin(DOUT1, SCK1);
  scale2.begin(DOUT2, SCK2);

  Serial.println("==========================================");
  Serial.println("        DUAL HX711 CALIBRATION");
  Serial.println("==========================================");

  Serial.println();
  Serial.println("HX711 #1:");
  Serial.println("T1/t1 = TARE");
  Serial.println("K1/k1 = RAW beban 10 kg");
  Serial.println("C1/c1 = Hitung CF");

  Serial.println();
  Serial.println("HX711 #2:");
  Serial.println("T2/t2 = TARE");
  Serial.println("K2/k2 = RAW beban 10 kg");
  Serial.println("C2/c2 = Hitung CF");

  Serial.println();

  // ==================================================
  // TARE AWAL HX711 #1
  // ==================================================
  rawT1 = scale1.read();
  raw_sebelumnya1 = rawT1;

  Serial.print("RAW T1 AWAL = ");
  Serial.println(rawT1);

  Serial.print("CF1 AWAL = ");
  Serial.println(calibrationFactor1, 3);


  // ==================================================
  // TARE AWAL HX711 #2
  // ==================================================
  rawT2 = scale2.read();
  raw_sebelumnya2 = rawT2;

  Serial.print("RAW T2 AWAL = ");
  Serial.println(rawT2);

  Serial.print("CF2 AWAL = ");
  Serial.println(calibrationFactor2, 3);

  Serial.println();
}


// ==================================================
// LOOP
// ==================================================
void loop()
{
  // ==================================================
  // PERINTAH SERIAL
  // ==================================================
  if (Serial.available())
  {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();


    // ==================================================
    // TARE HX711 #1
    // ==================================================
    if (cmd == "T1" || cmd == "t1")
    {
      rawT1 = scale1.read();
      raw_sebelumnya1 = rawT1;

      Serial.println();
      Serial.println("========== TARE HX711 #1 ==========");

      Serial.print("RAW T1 = ");
      Serial.println(rawT1);

      Serial.println("Massa 1 = 0.0 kg");

      Serial.println("====================================");
      Serial.println();
    }


    // ==================================================
    // BEBAN 10 KG HX711 #1
    // ==================================================
    else if (cmd == "K1" || cmd == "k1")
    {
      rawK1 = scale1.read();

      Serial.println();
      Serial.println("======= BEBAN 10 KG HX711 #1 =======");

      Serial.print("RAW K1 = ");
      Serial.println(rawK1);

      Serial.print("Massa diketahui = ");
      Serial.print(MASSA_KALIBRASI, 1);
      Serial.println(" kg");

      Serial.println("====================================");
      Serial.println();
    }


    // ==================================================
    // HITUNG CF HX711 #1
    // ==================================================
    else if (cmd == "C1" || cmd == "c1")
    {
      long perubahanRaw1 = rawK1 - rawT1;

      calibrationFactor1 =
        (float)perubahanRaw1 / MASSA_KALIBRASI;

      Serial.println();
      Serial.println("======= HASIL KALIBRASI HX711 #1 =======");

      Serial.print("RAW T1 = ");
      Serial.println(rawT1);

      Serial.print("RAW K1 = ");
      Serial.println(rawK1);

      Serial.print("Perubahan RAW 1 = ");
      Serial.println(perubahanRaw1);

      Serial.print("Massa = ");
      Serial.print(MASSA_KALIBRASI, 1);
      Serial.println(" kg");

      Serial.print("CALIBRATION FACTOR 1 = ");
      Serial.println(calibrationFactor1, 3);

      Serial.println("=========================================");
      Serial.println();
    }


    // ==================================================
    // TARE HX711 #2
    // ==================================================
    else if (cmd == "T2" || cmd == "t2")
    {
      rawT2 = scale2.read();
      raw_sebelumnya2 = rawT2;

      Serial.println();
      Serial.println("========== TARE HX711 #2 ==========");

      Serial.print("RAW T2 = ");
      Serial.println(rawT2);

      Serial.println("Massa 2 = 0.0 kg");

      Serial.println("====================================");
      Serial.println();
    }


    // ==================================================
    // BEBAN 10 KG HX711 #2
    // ==================================================
    else if (cmd == "K2" || cmd == "k2")
    {
      rawK2 = scale2.read();

      Serial.println();
      Serial.println("======= BEBAN 10 KG HX711 #2 =======");

      Serial.print("RAW K2 = ");
      Serial.println(rawK2);

      Serial.print("Massa diketahui = ");
      Serial.print(MASSA_KALIBRASI, 1);
      Serial.println(" kg");

      Serial.println("====================================");
      Serial.println();
    }


    // ==================================================
    // HITUNG CF HX711 #2
    // ==================================================
    else if (cmd == "C2" || cmd == "c2")
    {
      long perubahanRaw2 = rawK2 - rawT2;

      calibrationFactor2 =
        (float)perubahanRaw2 / MASSA_KALIBRASI;

      Serial.println();
      Serial.println("======= HASIL KALIBRASI HX711 #2 =======");

      Serial.print("RAW T2 = ");
      Serial.println(rawT2);

      Serial.print("RAW K2 = ");
      Serial.println(rawK2);

      Serial.print("Perubahan RAW 2 = ");
      Serial.println(perubahanRaw2);

      Serial.print("Massa = ");
      Serial.print(MASSA_KALIBRASI, 1);
      Serial.println(" kg");

      Serial.print("CALIBRATION FACTOR 2 = ");
      Serial.println(calibrationFactor2, 3);

      Serial.println("=========================================");
      Serial.println();
    }
  }


  // ==================================================
  // BACA RAW HX711 #1
  // ==================================================
  long raw1 = scale1.read();

  long perubahanRaw1 =
    raw1 - raw_sebelumnya1;

  float kg1 =
    hitungKg1(raw1);

  float perubahanKg1 =
    (float)perubahanRaw1 / calibrationFactor1;


  // ==================================================
  // BACA RAW HX711 #2
  // ==================================================
  long raw2 = scale2.read();

  long perubahanRaw2 =
    raw2 - raw_sebelumnya2;

  float kg2 =
    hitungKg2(raw2);

  float perubahanKg2 =
    (float)perubahanRaw2 / calibrationFactor2;


  // ==================================================
  // SERIAL HX711 #1
  // ==================================================
 // Serial.print("HX1 | RAW: ");
 // Serial.print(raw1);

 // Serial.print(" | dRAW: ");
 // Serial.print(perubahanRaw1);

  Serial.print(" | Massa1: ");
  Serial.print(kg1, 1);
  Serial.print(" kg");

  Serial.print(" | dMassa1: ");
  Serial.print(perubahanKg1, 1);
  Serial.print(" kg");

 // Serial.print(" | T1: ");
 // Serial.print(rawT1);

 // Serial.print(" | CF1: ");
 // Serial.print(calibrationFactor1, 3);


  // ==================================================
  // SERIAL HX711 #2
  // ==================================================
  //Serial.print(" || HX2 | RAW: ");
  //Serial.print(raw2);

 // Serial.print(" | dRAW: ");
 // Serial.print(perubahanRaw2);

  Serial.print(" | Massa2: ");
  Serial.print(kg2, 1);
  Serial.print(" kg");

  Serial.print(" | dMassa: ");
  Serial.print(perubahanKg2, 1);
  Serial.print(" kg");

  //Serial.print(" | T2: ");
  //Serial.print(rawT2);

  //Serial.print(" | CF2: ");
  //Serial.println(calibrationFactor2, 3);

 Serial.print(" | ALL: ");
  Serial.println((kg1+kg2)/2, 3);

  // ==================================================
  // SIMPAN RAW SEBELUMNYA
  // ==================================================
  raw_sebelumnya1 = raw1;
  raw_sebelumnya2 = raw2;

  delay(1000);
}