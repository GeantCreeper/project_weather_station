/*
  Station météo ESP32 – projet intégral
  -------------------------------------------------------------
  Capteurs :
    • CCS811   (qualité de l’air eCO₂ / TVOC)    – I²C @0x5A
    • BH1750   (luxmètre)                       – I²C @0x5C
    • BMP280   (température / pression)         – I²C @0x76
    • KY‑024    (anémomètre – Hall)             – DIG 4 + ANA 35
  Actionneurs :
    • Bandeau NeoPixel 8 LED                    – DATA 18  (5 V)
    • Buzzer MH‑FMD                             – IO 5
    • Afficheur OLED 0,96" SSD1306              – I²C @0x3C
  Bus I²C commun : SDA 21 / SCL 22

  © 2025 – À adapter librement. Les blocs « intouchables » (anciens sketches)
  ont été conservés tels quels, simplement encapsulés dans des fonctions afin
  de s’intégrer proprement dans un seul programme.
*/


#include <Wire.h>
#include <Adafruit_CCS811.h>
#include <BH1750.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "driver/ledc.h"
void tone(uint8_t pin, unsigned int frequency, unsigned long duration = 0) {
  ledcAttachPin(pin, 0);
  ledcWriteTone(0, frequency);
  if (duration > 0) {
    delay(duration);
    ledcDetachPin(pin);
  }
}
void noTone(uint8_t pin) {
  ledcDetachPin(pin);
}

#define PIN_SDA          21
#define PIN_SCL          22
#define PIN_BUZZER        5
#define PIN_NEOPIXEL     18
#define N_PIXELS          8
#define PIN_ANEMO_DIG     4
#define PIN_ANEMO_ANALOG 35

Adafruit_CCS811    ccs;
BH1750             luxMeter;
Adafruit_BMP280    bmp;
Adafruit_SSD1306   display(128, 64, &Wire, -1);
Adafruit_NeoPixel  strip(N_PIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

volatile uint32_t pulseCount = 0;
volatile uint32_t lastMicros  = 0;
const float  K_M_S    = 0.314;   //convertit Hz → m/s
const uint32_t DEBOUNCE_US = 5000; // >5 ms 
void IRAM_ATTR onPulse() {
  uint32_t now = micros();
  if (now - lastMicros > DEBOUNCE_US) {
    pulseCount++;
    lastMicros = now;
  }
}

const char* WIFI_SSID     = "node-bread";    // <‑‑‑ À renseigner
const char* WIFI_PASSWORD = "breadnode";
const char* MQTT_SERVER   = "mqtt.ci-ciad.utbm.fr:1883"; // <‑‑‑ IP du broker
const char* MQTT_TOPIC_CMD = "buzzer/cmd";

WiFiClient      espClient;
PubSubClient    mqttClient(espClient);

#define ECO2_THRESHOLD   1000   // ppm
#define TVOC_THRESHOLD    500   // ppb
#define TEMP_HIGH_THR      35.0 // °C
#define TEMP_LOW_THR        0.0 // °C
#define WIND_HIGH_THR      10.0 // m/s

const uint32_t SENSOR_PERIOD_MS = 2000; 

void connectWiFi();
void connectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void updateDisplay(float t, float p, float lux, uint16_t eco2, uint16_t tvoc, float wind);
void updatePixels(uint16_t eco2);
void alertIfNeeded(uint16_t eco2, uint16_t tvoc, float t, float wind);

// temps
uint32_t lastSensorMs = 0;

void setup() {
  // --- Console ---
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\n== Station météo ESP32 – démarrage =="));

  // --- I²C ---
  Wire.begin(PIN_SDA, PIN_SCL);

  // --- Capteur CCS811 (qualité d’air) — code inchangé encapsulé
  if (!ccs.begin()) {
    Serial.println(F("CCS811 introuvable "));
    while (true) delay(1000);
  }
  Serial.println(F("CCS811 chauffe"));
  while (!ccs.available()) delay(50);
  Serial.println(F("CCS811 prêt"));

  // --- BH1750 (lux) — code inchangé encapsulé
  if (!luxMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C, &Wire)) {
    Serial.println(F("BH1750 KO"));
    while (true) delay(1000);
  }

  // --- BMP280 (temp / pression)
  if (!bmp.begin(0x76)) {
    Serial.println(F("BMP280 KO "));
    while (true) delay(1000);
  }
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);

  // --- Anémomètre (Hall)
  pinMode(PIN_ANEMO_DIG, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ANEMO_DIG), onPulse, FALLING);
  analogReadResolution(12);

  // --- NeoPixel ---
  strip.begin();
  strip.show(); // tout éteint

  // --- Buzzer ---
  pinMode(PIN_BUZZER, OUTPUT);

  // --- OLED ---
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 KO "));
    while (true) delay(1000);
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Station meteo");
  display.display();

  // --- Wi‑Fi & MQTT ---
  connectWiFi();
  mqttClient.setServer(MQTT_SERVER, 1883);
  mqttClient.setCallback(mqttCallback);
  connectMQTT();
}

void loop() {
  // gestion MQTT continue
  if (!mqttClient.connected()) connectMQTT();
  mqttClient.loop();

  // lecture cyclique des capteurs
  uint32_t now = millis();
  if (now - lastSensorMs >= SENSOR_PERIOD_MS) {
    lastSensorMs = now;

    // — CCS811
    uint16_t eco2 = 0, tvoc = 0;
    if (ccs.available() && !ccs.readData()) {
      eco2 = ccs.geteCO2();
      tvoc = ccs.getTVOC();
    }

    // — BH1750
    float lux = luxMeter.readLightLevel();

    // — BMP280
    float tempC = bmp.readTemperature();
    float press = bmp.readPressure() / 100.0; // hPa

    // — Anémomètre
    noInterrupts();
    uint32_t pulses = pulseCount;
    pulseCount = 0;
    interrupts();
    float freqHz = pulses / (SENSOR_PERIOD_MS / 1000.0);
    float windMS = freqHz * K_M_S;
    uint16_t hallRaw = analogRead(PIN_ANEMO_ANALOG);

    // --- Affichage & feedbacks ---
    updateDisplay(tempC, press, lux, eco2, tvoc, windMS);
    updatePixels(eco2);
    alertIfNeeded(eco2, tvoc, tempC, windMS);

    // Debug console
    Serial.printf("T:%.2f °C  P:%.1f hPa  Lux:%.0f lx  eCO2:%u ppm  TVOC:%u ppb  Vent:%.2f m/s  Hall:%u\n",
                  tempC, press, lux, eco2, tvoc, windMS, hallRaw);
  }
}

void connectWiFi() {
  Serial.printf("Connexion Wi‑Fi à %s…\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint8_t tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print('.');
    tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" connecté !");
    Serial.print("IP : "); Serial.println(WiFi.localIP());
  } else {
    Serial.println(" KO – continue hors‑ligne.");
  }
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connexion MQTT…");
    if (mqttClient.connect("ESP32_Station")) {
      Serial.println(" OK");
      mqttClient.subscribe(MQTT_TOPIC_CMD);
    } else {
      Serial.print(" échec, code : ");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();
  if (msg.equalsIgnoreCase("ON")) {
    tone(PIN_BUZZER, 1000, 300);
  } else if (msg.equalsIgnoreCase("OFF")) {
    noTone(PIN_BUZZER);
  }
}

void updateDisplay(float t, float p, float lux, uint16_t eco2, uint16_t tvoc, float wind) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.printf("T:%5.1fC\nP:%6.1fhPa\nLux:%5.0flx\neCO2:%4uppm\nTVOC:%4uppb\nWind:%4.1fm/s", t, p, lux, eco2, tvoc, wind);
  display.display();
}

void updatePixels(uint16_t eco2) {
  uint32_t color;
  if (eco2 < 800)       color = strip.Color(0, 150, 0);   // vert
  else if (eco2 < 1200) color = strip.Color(150, 150, 0); // jaune
  else                  color = strip.Color(150, 0, 0);   // rouge
  for (uint8_t i = 0; i < N_PIXELS; i++) strip.setPixelColor(i, color);
  strip.show();
}

void alertIfNeeded(uint16_t eco2, uint16_t tvoc, float t, float wind) {
  static uint32_t lastBeep = 0;
  uint32_t now = millis();
  bool alert = (eco2 > ECO2_THRESHOLD) || (tvoc > TVOC_THRESHOLD) || (t < TEMP_LOW_THR) || (t > TEMP_HIGH_THR) || (wind > WIND_HIGH_THR);
  if (alert && (now - lastBeep > 10000)) {  // un bip max toutes les 10 s
    tone(PIN_BUZZER, 2000, 200);
    lastBeep = now;
  }
}
