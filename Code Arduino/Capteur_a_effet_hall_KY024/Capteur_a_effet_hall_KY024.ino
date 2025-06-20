/************************************************************
 *  Anémomètre – capteur KY-024 (Hall) – ESP32
 *  A0  -> GPIO 35 (ADC1_CH7)
 *  D0  -> GPIO 4  (interruption)
 *  VCC -> 3V3
 *  GND -> GND
 ************************************************************/
#define PIN_ANEMO_DIG 4
#define PIN_HALL_ANALOG 35

volatile uint32_t pulseCount = 0;
volatile uint32_t lastMicros = 0;
const uint32_t SAMPLE_MS = 2000;
const float    K_M_S     = 0.314;   // 2πr avec r = 50 mm (à ajuster)

void IRAM_ATTR onPulse(){
  uint32_t now = micros();
  if (now - lastMicros > 5000){     // >5 ms = <200 Hz
    pulseCount++;
    lastMicros = now;
  }
}

void setup(){
  Serial.begin(115200);
  pinMode(PIN_ANEMO_DIG, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ANEMO_DIG), onPulse, FALLING);
  analogReadResolution(12);
}

void loop(){
  static uint32_t t0 = millis();
  if (millis() - t0 >= SAMPLE_MS){
    noInterrupts();
    uint32_t pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    float freqHz = pulses / (SAMPLE_MS / 1000.0);
    float windMS = freqHz * K_M_S;
    uint16_t hallRaw = analogRead(PIN_HALL_ANALOG);

    Serial.printf("Pulses: %5u | %.2f Hz | %.2f m/s | Analog: %u\n",
                  pulses, freqHz, windMS, hallRaw);
    t0 = millis();
  }
}
