/*****************************************************************
 *  BMP280  –  ESP32 (I²C, T° + Pression seulement)
 *
 *  Câblage (mode I²C) :
 *    BMP280 VIN  → 3V3
 *    BMP280 GND  → GND
 *    BMP280 SCL  → GPIO 22
 *    BMP280 SDA  → GPIO 21
 *    BMP280 CS   → 3V3      // force l’I²C
 *    BMP280 SDO  → GND      // adresse 0x76  (ou 3V3 pour 0x77)
 *
 *  Bibliothèque à installer : « Adafruit BMP280 »
 *****************************************************************/
#include <Wire.h>
#include <Adafruit_BMP280.h>

// --- Adresse I²C selon SDO ---
#define BMP280_ADDR 0x76        // 0x76 (SDO→GND) ou 0x77 (SDO→3V3)

// --- Broches I²C (modifiables) ---
constexpr uint8_t SDA_PIN = 21; // data
constexpr uint8_t SCL_PIN = 22; // clock

Adafruit_BMP280 bmp;            // instance capteur

void setup() {
  Serial.begin(115200);
  delay(200);                   // laisse le temps au port série

  Wire.begin(SDA_PIN, SCL_PIN); // initialise le bus I²C

  if (!bmp.begin(BMP280_ADDR)) {
    Serial.println(F("❌  BMP280 introuvable – vérifiez câblage et adresse !"));
    while (1) delay(10);
  }
  Serial.println(F("✅  BMP280 détecté – mesures T°/P en cours…"));

  // Réglages optionnels (décommenter pour modifier)
  /*
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,   // sur-échantillonnage T°
                  Adafruit_BMP280::SAMPLING_X16,  // sur-échantillonnage P
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);
  */
}

void loop() {
  float temperature = bmp.readTemperature();      // °C
  float pressure    = bmp.readPressure() / 100.0; // hPa

  Serial.printf("T = %.2f °C   |   P = %.2f hPa\n",
                temperature, pressure);

  delay(1000);  // 1 mesure / seconde
}
