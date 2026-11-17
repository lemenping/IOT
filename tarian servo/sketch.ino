#include <ESP32Servo.h>
#include <math.h>

const int NUM_SERVOS = 12;

// Urutan melingkar searah jarum jam (Clockwise) di sekeliling ESP32:
// Atas (13, 12, 14, 27) -> Kanan (33, 32) -> Bawah (16, 4, 2, 15) -> Kiri (25, 26)
const int servoPins[NUM_SERVOS] = {
  13, 12, 14, 27, // Atas (kiri ke kanan)
  33, 32,         // Kanan (atas ke bawah)
  16, 4, 2, 15,   // Bawah (kanan ke kiri)
  25, 26          // Kiri (bawah ke atas)
};

Servo servos[NUM_SERVOS];

void setup() {
  Serial.begin(115200);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].setPeriodHertz(50);
    servos[i].attach(servoPins[i], 500, 2400);
    servos[i].write(90);
  }

  Serial.println("=== TARIAN 12 SERVO DIMULAI ===");
  delay(1000);
}

// 1. Tarian Gelombang Melingkar (Smooth Sine Wave)
void tarianGelombangMelingkar(int putaran) {
  Serial.println("1. Gelombang Melingkar");
  for (int t = 0; t < 360 * putaran; t += 6) {
    for (int i = 0; i < NUM_SERVOS; i++) {
      float sudutRad = radians(t + (i * 30)); // Beda fase 30 derajat tiap servo
      int posisi = 90 + (int)(65 * sin(sudutRad));
      servos[i].write(posisi);
    }
    delay(15);
  }
}

// 2. Tarian Cermin (Kiri & Kanan Bergerak Simetris Berlawanan)
void tarianCermin(int ulang) {
  Serial.println("2. Tarian Cermin Simetris");
  for (int u = 0; u < ulang; u++) {
    for (int sudut = 20; sudut <= 160; sudut += 3) {
      for (int i = 0; i < NUM_SERVOS; i++) {
        // Sisi kanan (indeks 2..7) berlawanan dengan sisi kiri
        if (i >= 2 && i <= 7) {
          servos[i].write(sudut);
        } else {
          servos[i].write(180 - sudut);
        }
      }
      delay(12);
    }
    for (int sudut = 160; sudut >= 20; sudut -= 3) {
      for (int i = 0; i < NUM_SERVOS; i++) {
        if (i >= 2 && i <= 7) {
          servos[i].write(sudut);
        } else {
          servos[i].write(180 - sudut);
        }
      }
      delay(12);
    }
  }
}

// 3. Tarian Efek Domino / Kipas Melingkar
void tarianDomino(int ulang) {
  Serial.println("3. Efek Domino Melingkar");
  for (int u = 0; u < ulang; u++) {
    // Buka satu per satu searah jarum jam
    for (int i = 0; i < NUM_SERVOS; i++) {
      servos[i].write(160);
      delay(60);
    }
    // Tutup kembali satu per satu
    for (int i = 0; i < NUM_SERVOS; i++) {
      servos[i].write(20);
      delay(60);
    }
  }
  // Kembali ke posisi netral
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].write(90);
    delay(40);
  }
}

// 4. Tarian Selang-Seling (Ganjil vs Genap)
void tarianSelangSeling(int ulang) {
  Serial.println("4. Tarian Ganjil-Genap");
  for (int u = 0; u < ulang; u++) {
    for (int i = 0; i < NUM_SERVOS; i++) {
      servos[i].write((i % 2 == 0) ? 150 : 30);
    }
    delay(350);
    for (int i = 0; i < NUM_SERVOS; i++) {
      servos[i].write((i % 2 == 0) ? 30 : 150);
    }
    delay(350);
  }
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].write(90);
  }
}

void loop() {
  tarianGelombangMelingkar(3);
  delay(300);

  tarianCermin(3);
  delay(300);

  tarianDomino(2);
  delay(300);

  tarianSelangSeling(4);
  delay(500);
}
