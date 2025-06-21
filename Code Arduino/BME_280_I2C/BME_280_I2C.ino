#include <Wire.h>
#include <Adafruit_BMP280.h>

// Déclaration du capteur BMP280 en I2C
Adafruit_BMP280 bmp;  

void setup() {
  Serial.begin(115200);
  if (!bmp.begin(0x76)) {                      // Initialise le BMP280 à l'adresse 0x76 (modifier en 0x77 si nécessaire)
    Serial.println("BMP280 introuvable, vérifier le câblage ou l'adresse !");
    while (1) delay(100);
  }
  // Configuration optionnelle : mode normal, oversampling x2 (temp) / x16 (press), filtre x16
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, 
                  Adafruit_BMP280::SAMPLING_X2,   // suréchantillonnage température
                  Adafruit_BMP280::SAMPLING_X16,  // suréchantillonnage pression
                  Adafruit_BMP280::FILTER_X16,    // filtre
                  Adafruit_BMP280::STANDBY_MS_500);
}

void loop() {
  float temperature = bmp.readTemperature();      // Lecture de la température en °C
  float pression = bmp.readPressure() / 100.0;    // Lecture de la pression en Pa, convertie en hPa
  Serial.print("Température = ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Pression = ");
  Serial.print(pression);
  Serial.println(" hPa");

  delay(2000);
}
