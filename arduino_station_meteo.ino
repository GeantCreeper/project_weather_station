/*  Station météo ESP32  –  version Node-RED 2025
 *  ------------------------------------------------
 *  Capteurs  : BME280  (T, H, P)  I²C 0x76
 *              CCS811  (eCO₂)     I²C 0x5A
 *              BH1750  (Lux)      I²C 0x5C
 *              Anémomètre Hall    DIG 4  (+ ANA 35 pour debug)
 *
 *  Actionneurs : NeoPixel 8 LED   GPIO18
 *                Buzzer           GPIO5
 *                OLED SSD1306     I²C 0x3C
 *
 *  MQTT  →  publie :
 *     capteur/bmp280   [temp, hum, press]                (toutes 5 s)
 *     capteur/Qair     { "temp": eCO2 }                  (5 s)
 *     capteur/lux      { "temp": lux  }                  (5 s)
 *     capteur/vent     { "temp": kmh  }                  (1 s)
 *
 *  MQTT  ←  écoute :
 *     buzzer/etat            "ON" | "OFF"
 *     led/indicator          { brightness, rgb:[[r,g,b]×8] }
 *     lcd/texte              texte libre  (2×16 car max conseillé)
 */

#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_BME280.h>
#include <Adafruit_CCS811.h>
#include <BH1750.h>

/*==========================  RÉGLAGES  ==========================*/
#define WIFI_SSID     "node-bread"
#define WIFI_PASS     "breadnode"

const char* MQTT_HOST = "mqtt.ci-ciad.utbm.fr";
const uint16_t MQTT_PORT = 1883;

/*––– I/O –––*/
#define PIN_SDA        21
#define PIN_SCL        22
#define PIN_BUZZER      5
#define PIN_PIXELS     18
#define N_PIXELS        8
#define PIN_WIND_DIG    4
#define PIN_WIND_ANA   35   // optionnel debug

/*––– Périodes –––*/
const uint32_t PERIOD_5S = 5000;
const uint32_t PERIOD_1S = 1000;

/*––– Anémomètre –––*/
const float PULSE_TO_KMH = 2.4f;     // ← adapte selon étalonnage
const uint32_t DEBOUNCE_US = 5000;

/*==========================  OBJETS  ============================*/
WiFiClient      wifi;
PubSubClient    mqtt(wifi);

Adafruit_BME280 bme;      // I²C 0x76
Adafruit_CCS811 ccs811;   // 0x5A
BH1750          bh1750;   // 0x5C
Adafruit_NeoPixel pixels(N_PIXELS, PIN_PIXELS, NEO_GRB + NEO_KHZ800);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

/*=====================  Variables globales  =====================*/
volatile uint32_t windPulses = 0;
volatile uint32_t lastPulseUs = 0;

unsigned long t5s = 0, t1s = 0;

/*=====================  Interrupt anémomètre  ===================*/
void IRAM_ATTR onWindPulse()
{
  uint32_t now = micros();
  if (now - lastPulseUs > DEBOUNCE_US) {
    windPulses++;
    lastPulseUs = now;
  }
}

/*=====================  Affichage OLED  =========================*/
void oledPrintCentered(const String& line1)
{
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.println(line1);
  oled.display();
}

/*=====================  MQTT callbacks  =========================*/
void applyPixelFrame(JsonArray rgb)
{
  for (uint8_t i = 0; i < N_PIXELS; i++) {
    if (i < rgb.size()) {
      pixels.setPixelColor(i,
        pixels.Color(rgb[i][0], rgb[i][1], rgb[i][2]));
    } else {
      pixels.setPixelColor(i, 0);
    }
  }
  pixels.show();
}

void mqttCallback(char* topic, byte* payload, unsigned int len)
{
  String t(topic);
  payload[len] = '\0';       // rendu exploitable

  if (t == "buzzer/etat") {           /*––– BUZZER –––*/
    if (payload[0]=='O' && payload[1]=='N')
      tone(PIN_BUZZER, 1000, 200);
    else
      noTone(PIN_BUZZER);
  }

  else if (t == "lcd/texte") {        /*––– OLED –––*/
    oledPrintCentered((char*)payload);
  }

  else if (t == "led/indicator") {    /*––– NeoPixel –––*/
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err) {
      if (doc.containsKey("brightness"))
        pixels.setBrightness(doc["brightness"]);
      if (doc["rgb"].is<JsonArray>())
        applyPixelFrame(doc["rgb"].as<JsonArray>());
    }
  }
}

/*=====================  Connexions  =============================*/
void connectWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status()!=WL_CONNECTED) delay(500);
}

void connectMQTT()
{
  while (!mqtt.connected()) {
    mqtt.connect("ESP32_Meteo");
    if (!mqtt.connected()) delay(2000);
  }
  mqtt.subscribe("buzzer/etat");
  mqtt.subscribe("led/indicator");
  mqtt.subscribe("lcd/texte");
}

/*=====================  Publication MQTT  =======================*/
void pubBmp280(float t,float h,float p)
{
  StaticJsonDocument<64> doc;
  JsonArray arr = doc.to<JsonArray>();
  arr.add(t); arr.add(h); arr.add(p);
  char buf[64]; size_t n = serializeJson(arr, buf);
  mqtt.publish("capteur/bmp280", buf, n);
}

void pubSingle(const char* topic, float value)
{
  StaticJsonDocument<32> doc;
  doc["temp"] = value;
  char buf[32]; size_t n = serializeJson(doc, buf);
  mqtt.publish(topic, buf, n);
}

/*=====================  SETUP  ==================================*/
void setup()
{
  Serial.begin(115200);
  Wire.begin(PIN_SDA, PIN_SCL);

  /* Capteurs */
  bme.begin(0x76);
  bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C);
  ccs811.begin();

  /* Anémomètre */
  pinMode(PIN_WIND_DIG, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_WIND_DIG), onWindPulse, FALLING);

  /* OLED & Pixels & buzzer */
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oledPrintCentered("Booting…");
  pixels.begin(); pixels.show();
  pinMode(PIN_BUZZER, OUTPUT); noTone(PIN_BUZZER);

  /* Réseaux */
  connectWiFi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  connectMQTT();
}

/*=====================  LOOP  ===================================*/
void loop()
{
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  unsigned long now = millis();

  /*——— Vent chaque seconde ———*/
  if (now - t1s >= PERIOD_1S) {
    t1s = now;
    noInterrupts();
    uint32_t pulses = windPulses;
    windPulses = 0;
    interrupts();

    float kmh = pulses * PULSE_TO_KMH;
    pubSingle("capteur/vent", kmh);
  }

  /*——— Tous les 5 s : BME + CCS811 + BH1750 ———*/
  if (now - t5s >= PERIOD_5S) {
    t5s = now;

    /* BME280 */
    float tempC = bme.readTemperature();
    float hum   = bme.readHumidity();
    float press = bme.readPressure() / 100.0f;  // hPa

    /* CCS811 */
    if (ccs811.available()) ccs811.readData();
    uint16_t eco2 = ccs811.geteCO2();

    /* Lux */
    float lux = bh1750.readLightLevel();

    /* Publier */
    pubBmp280(tempC, hum, press);
    pubSingle("capteur/Qair", eco2);
    pubSingle("capteur/lux", lux);

    /* Retour console */
    Serial.printf("T %.1f  H %.1f  P %.1f  eCO2 %u  Lux %.0f\n",
                  tempC, hum, press, eco2, lux);
  }
}
