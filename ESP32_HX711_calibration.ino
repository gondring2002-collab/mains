#include <Arduino.h>
#include "HX711.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 19; //15
const int LOADCELL_SCK_PIN  = 18; //4

HX711 scale;

bool tareDone = false;

void setup() {
  Serial.begin(115200);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  Serial.println();
  Serial.println("=== HX711 Calibration ===");
  Serial.println("T = Tare (tanpa beban)");
  Serial.println("K = Ambil pembacaan beban");
  Serial.println();
}

void loop() {

  if (Serial.available()) {

    char cmd = toupper(Serial.read());

    if (cmd == 'T') {

      Serial.println();
      Serial.println("Pastikan tidak ada beban.");
      Serial.println("Melakukan tare...");

      scale.set_scale();
      scale.tare();

      tareDone = true;

      Serial.println("Tare selesai.");
      Serial.println("Pasang beban referensi.");
    }

    else if (cmd == 'K') {

      if (!tareDone) {
        Serial.println("Lakukan T terlebih dahulu.");
        return;
      }

      Serial.println();
      Serial.println("Mengambil data...");

      long reading = scale.get_units(10);

      Serial.print("Result = ");
      Serial.println(reading);

      Serial.println();
      Serial.println("Calibration Factor = Result / Berat Referensi");
    }
  }
}