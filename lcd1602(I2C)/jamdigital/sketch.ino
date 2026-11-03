#include <LiquidCrystal_I2C.h>

// Inisialisasi LCD dengan alamat I2C 0x27, ukuran 16x2
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variabel untuk menyimpan waktu (mulai dari jam 12:00:00)
int jam = 12;
int menit = 0;
int detik = 0;

void setup() {
  lcd.init();
  lcd.backlight();
  
  // Tampilkan label statis di baris pertama
  lcd.setCursor(2, 0);
  lcd.print("JAM DIGITAL");
}

void loop() {
  // Format tampilan waktu agar selalu 2 digit (contoh: 09:05:03)
  lcd.setCursor(4, 1);
  
  if (jam < 10) lcd.print("0");
  lcd.print(jam);
  lcd.print(":");
  
  if (menit < 10) lcd.print("0");
  lcd.print(menit);
  lcd.print(":");
  
  if (detik < 10) lcd.print("0");
  lcd.print(detik);

  // Logika penambahan waktu setiap 1 detik (1000 milidetik)
  delay(1000);
  detik++;

  // Logika pergantian waktu
  if (detik >= 60) {
    detik = 0;
    menit++;
    if (menit >= 60) {
      menit = 0;
      jam++;
      if (jam >= 24) {
        jam = 0; // Kembali ke jam 00 setelah 24 jam
      }
    }
  }
}
