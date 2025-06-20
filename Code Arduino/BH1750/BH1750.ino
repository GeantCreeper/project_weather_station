/*
  Lecture d’un capteur SEN0097 (BH1750) avec une carte ESP32
  — Mesure de l’éclairement lumineux en lux
  — Branchement :
        SEN0097 VCC  → 3,3 V (ou 5 V si votre breakout l’accepte)
        SEN0097 GND  → GND
        SEN0097 SDA  → GPIO 21  (SDA)
        SEN0097 SCL  → GPIO 22  (SCL)
        SEN0097 ADO  → 3,3 V    (force l’adresse I²C à 0x5C)
  — Bibliothèque : BH1750 (auteur : Christopher Laws), installable via
    *Sketch ▸ Include Library ▸ Manage Libraries…* puis rechercher “BH1750”.
*/

#include <Wire.h>
#include <BH1750.h>

// Objet capteur ; on précisera l’adresse dans begin()
BH1750 luxMeter;

void setup() {
  Serial.begin(115200);
  delay(300);

  // Initialise le bus I²C : Wire.begin(SDA, SCL)
  Wire.begin(21, 22);

  // Démarrage du BH1750 en mode « haute résolution continue » (1 lx, 120 ms)
  // Adresse 0x5C car ADO est relié au 3,3 V
  if (!luxMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C, &Wire)) {
    Serial.println(F("❌  Impossible de démarrer le BH1750 ! Vérifiez le câblage."));
    while (true) { delay(1000); }
  }

  Serial.println(F("✅  BH1750 prêt — lecture en cours…"));
}

void loop() {
  // readLightLevel() retourne un float en lux ; NAN si lecture invalide
  float lux = luxMeter.readLightLevel();

  if (!isnan(lux)) {
    Serial.print(F("Luminosité : "));
    Serial.print(lux);
    Serial.println(F(" lux"));
  } else {
    Serial.println(F("⚠️  Lecture invalide (capteur occupé ?)"));
  }

  delay(1000);   // 1 mesure par seconde
}
