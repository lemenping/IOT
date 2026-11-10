#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// ==========================================
// DEFINISI PIN GPIO ESP32
// ==========================================
#define PIN_TRIG        5
#define PIN_ECHO        18
#define PIN_DHT         15
#define DHTTYPE         DHT22

#define PIN_RELAY       26
#define PIN_BUZZER      25
#define PIN_LED_GREEN   14
#define PIN_LED_YELLOW  27
#define PIN_LED_RED     13

// ==========================================
// KONSTANTA KALIBRASI TANGKI AIR (DALAM CM)
// ==========================================
const float DIST_FULL_CM  = 20.0;
const float DIST_EMPTY_CM = 220.0;
const float MIN_VALID_CM  = 2.0;
const float MAX_VALID_CM  = 350.0;

// ==========================================
// OBJEK PERANGKAT KERAS
// ==========================================
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(PIN_DHT, DHTTYPE);

// ==========================================
// VARIABEL STATUS SISTEM
// ==========================================
float distanceCm      = 0.0;
int   waterLevelPct   = 0;
float temperatureC    = 28.5;
float humidityPct     = 65.0;

bool  pumpState       = false;
bool  ultrasonicError = false;
bool  dhtError        = false;

unsigned long lastSensorMillis  = 0;
unsigned long lastDhtMillis     = 0;
unsigned long lastLcdMillis     = 0;
unsigned long lastPageMillis    = 0;
unsigned long lastAlarmMillis   = 0;

int  lcdPage          = 0;
bool buzzerToggle     = false;

// ==========================================
// FUNGSI PEMBACAAN SENSOR ULTRASONIK
// ==========================================
float readUltrasonicDistance() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duration = pulseIn(PIN_ECHO, HIGH, 30000);
  if (duration == 0) {
    return -1.0;
  }
  return (duration * 0.0343) / 2.0;
}

// ==========================================
// SETUP AWAL
// ==========================================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);

  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_YELLOW, LOW);
  digitalWrite(PIN_LED_RED, LOW);
  noTone(PIN_BUZZER);

  // Inisialisasi Sensor DHT22
  dht.begin();

  // Inisialisasi LCD 16x2 I2C
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("SMART WATER TANK");
  lcd.setCursor(0, 1);
  lcd.print("SYSTEM INIT...  ");
  delay(1500);

  // Baca DHT22 pertama kali setelah inisialisasi selesai
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    temperatureC = t;
    humidityPct  = h;
    dhtError     = false;
  }

  lcd.clear();
  Serial.println("==========================================");
  Serial.println("SMART WATER TANK AUTOMATION SYSTEM - ESP32");
  Serial.println("==========================================");
}

// ==========================================
// LOOP UTAMA (NON-BLOCKING)
// ==========================================
void loop() {
  unsigned long currentMillis = millis();

  // 1. BACA SENSOR SUHU & KELEMBAPAN DHT22 (Setiap 2000 ms)
  if (currentMillis - lastDhtMillis >= 2000) {
    lastDhtMillis = currentMillis;
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
      dhtError = true;
    } else {
      dhtError = false;
      temperatureC = t;
      humidityPct  = h;
    }
  }

  // 2. BACA SENSOR ULTRASONIK & LOGIKA OTOMATISASI (Setiap 500 ms)
  if (currentMillis - lastSensorMillis >= 500) {
    lastSensorMillis = currentMillis;

    distanceCm = readUltrasonicDistance();

    if (distanceCm < MIN_VALID_CM || distanceCm > MAX_VALID_CM) {
      ultrasonicError = true;
      pumpState = false; // Fail-safe: Pompa langsung OFF
    } else {
      ultrasonicError = false;

      float rawPct = ((DIST_EMPTY_CM - distanceCm) / (DIST_EMPTY_CM - DIST_FULL_CM)) * 100.0;
      waterLevelPct = constrain((int)round(rawPct), 0, 100);

      // Logika Hysteresis
      if (waterLevelPct <= 25) {
        pumpState = true;  // Air <= 25%: Pompa ON
      } else if (waterLevelPct >= 90) {
        pumpState = false; // Air >= 90%: Pompa OFF
      }
    }

    // Output Relay
    digitalWrite(PIN_RELAY, pumpState ? HIGH : LOW);

    // Output LED Indikator
    if (ultrasonicError) {
      digitalWrite(PIN_LED_GREEN, LOW);
      digitalWrite(PIN_LED_YELLOW, LOW);
    } else {
      digitalWrite(PIN_LED_RED, (waterLevelPct == 100) ? HIGH : LOW);
      digitalWrite(PIN_LED_YELLOW, pumpState ? HIGH : LOW);
      digitalWrite(PIN_LED_GREEN, (waterLevelPct < 100) ? HIGH : LOW);
    }

    // Tampilkan Log Serial Monitor
    Serial.print("[DATA] Jarak: ");
    if (ultrasonicError) {
      Serial.print("ERROR");
    } else {
      Serial.print(distanceCm, 1);
      Serial.print(" cm | Level: ");
      Serial.print(waterLevelPct);
      Serial.print("%");
    }
    Serial.print(" | Pompa: ");
    Serial.print(pumpState ? "ON" : "OFF");
    Serial.print(" | Suhu: ");
    if (dhtError) {
      Serial.println("ERROR");
    } else {
      Serial.print(temperatureC, 1);
      Serial.print("C | Lembap: ");
      Serial.print(humidityPct, 1);
      Serial.println("%");
    }
  }

  // 3. KONTROL ALARM BUZZER & KEDIP ERROR (Setiap 400 ms)
  if (currentMillis - lastAlarmMillis >= 400) {
    lastAlarmMillis = currentMillis;
    buzzerToggle = !buzzerToggle;

    if (ultrasonicError || dhtError) {
      if (buzzerToggle) {
        tone(PIN_BUZZER, 1500);
        if (ultrasonicError) digitalWrite(PIN_LED_RED, HIGH);
      } else {
        noTone(PIN_BUZZER);
        if (ultrasonicError) digitalWrite(PIN_LED_RED, LOW);
      }
    } else if (waterLevelPct == 100) {
      if (buzzerToggle) {
        tone(PIN_BUZZER, 1000);
      } else {
        noTone(PIN_BUZZER);
      }
    } else {
      noTone(PIN_BUZZER);
    }
  }

  // 4. PERGANTIAN HALAMAN LCD (Setiap 3500 ms)
  if (currentMillis - lastPageMillis >= 3500) {
    lastPageMillis = currentMillis;
    lcdPage = (lcdPage + 1) % 2;
  }

  // 5. UPDATE TAMPILAN LCD 16x2 (Setiap 500 ms)
  if (currentMillis - lastLcdMillis >= 500) {
    lastLcdMillis = currentMillis;
    char line0[17];
    char line1[17];

    if (ultrasonicError) {
      snprintf(line0, sizeof(line0), "!! SENSOR ERR !!");
      snprintf(line1, sizeof(line1), "CHECK ULTRASONIC");
    } else if (dhtError && lcdPage == 1) {
      snprintf(line0, sizeof(line0), "SMART WATER TANK");
      snprintf(line1, sizeof(line1), "DHT22 SENSOR ERR");
    } else if (lcdPage == 0) {
      snprintf(line0, sizeof(line0), "SMART WATER TANK");
      if (waterLevelPct == 100) {
        snprintf(line1, sizeof(line1), "LVL:100%% PMP:%-3s", pumpState ? "ON" : "OFF");
      } else {
        snprintf(line1, sizeof(line1), "LEVEL:%2d%% PMP:%-2s", waterLevelPct, pumpState ? "ON" : "OFF");
      }
    } else {
      if (waterLevelPct == 100) {
        snprintf(line0, sizeof(line0), "STATUS:TANK FULL");
      } else if (waterLevelPct <= 25) {
        snprintf(line0, sizeof(line0), "STATUS:LOW WATER");
      } else {
        snprintf(line0, sizeof(line0), "STATUS:NORMAL   ");
      }
      snprintf(line1, sizeof(line1), "T:%4.1fC H:%4.1f%% ", temperatureC, humidityPct);
    }

    lcd.setCursor(0, 0);
    lcd.print(line0);
    lcd.setCursor(0, 1);
    lcd.print(line1);
  }
}
