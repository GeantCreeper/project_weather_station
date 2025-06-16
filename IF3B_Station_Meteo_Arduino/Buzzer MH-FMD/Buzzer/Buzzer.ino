#include <WiFi.h>
#include <PubSubClient.h>

// ----- CONFIGURATION -----
#define BUZZER_PIN 25          // Broche du buzzer
const char* ssid = "";
const char* password = "";
const char* mqtt_server = ""; // Adresse IP du broker MQTT (Node-RED !)
const char* topicCmd = "buzzer/cmd";       // Topic MQTT à écouter

WiFiClient espClient;
PubSubClient client(espClient);

// ---- Fonctions pour ESP32 : tone/noTone
#include "driver/ledc.h"
void tone(uint8_t pin, unsigned int frequency, unsigned long duration = 0) {
  ledcAttachPin(pin, 0);
  ledcWriteTone(0, frequency);
  if (duration > 0) {
    delay(duration);
    noTone(pin);
  }
}
void noTone(uint8_t pin) {
  ledcDetachPin(pin);
}

// ---- Callback MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  String cmd;
  for (unsigned int i = 0; i < length; i++) {
    cmd += (char)payload[i];
  }
  cmd.trim();
  if (cmd.equalsIgnoreCase("ON")) {
    tone(BUZZER_PIN, 1000, 300); // Bip 300 ms
  } else if (cmd.equalsIgnoreCase("OFF")) {
    noTone(BUZZER_PIN);
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Buzzer")) {
      client.subscribe(topicCmd);
    } else {
      delay(2000);
    }
  }
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
