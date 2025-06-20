/*
  Lecture d’un capteur CCS811 V1 avec une carte ESP32
  — Mesure de la qualité d’air : eCO2 (ppm) + TVOC (ppb)
  — Branchement :
        CCS811 Vin  → 5 V (ou 3,3 V si votre breakout n’a pas de régulateur)
        CCS811 GND  → GND
        CCS811 SDA  → GPIO 21  (SDA)
        CCS811 SCL  → GPIO 22  (SCL)
        CCS811 nWAKE→ GND      (réveil permanent ; sinon piloter le pin)
  — Bibliothèque : Adafruit_CCS811 (à installer via le Library Manager)
*/

#include <Wire.h>
#include <Adafruit_CCS811.h>

Adafruit_CCS811 ccs;          // objet capteur

void setup() {
  Serial.begin(115200);
  delay(500);

  // Initialise le bus I2C de l’ESP32 : Wire.begin(SDA, SCL)
  Wire.begin(21, 22);

  // Démarrage du CCS811
  if (!ccs.begin()) {
    Serial.println(F("❌  Impossible de détecter le CCS811 !"));
    while (true) { delay(1000); }
  }

  // Attendre que le capteur commence ses mesures
  Serial.println(F("⏳  Mise en route du capteur…"));
  while (!ccs.available()) { delay(50); }
  Serial.println(F("✅  Capteur prêt !"));
}

void loop() {
  // Vérifie qu’une nouvelle mesure est disponible
  if (ccs.available()) {

    // Lit les données ; readData() renvoie true si ERREUR
    if (!ccs.readData()) {
      uint16_t eco2  = ccs.geteCO2();   // eCO₂ en ppm
      uint16_t tvoc  = ccs.getTVOC();   // TVOC en ppb

      Serial.print(F("eCO₂ : "));
      Serial.print(eco2);
      Serial.print(F(" ppm | TVOC : "));
      Serial.print(tvoc);
      Serial.println(F(" ppb"));
    } else {
      Serial.println(F("⚠️  Erreur de lecture — retour STATUS/ERROR"));
    }
  }

  delay(2000);   // lecture toutes les 2 s (minimum recommandé : 1 s)
}
