#define I2C_ADDR    0x27
#define LCD_COLUMNS 20
#define LCD_LINES   4
#define SDA_PIN A4
#define SCL_PIN A5
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>
#include <task.h>

// Inisialisasi LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Ganti 0x27 dengan alamat I2C perangkat Anda

void lcdTask(void *pvParameters) {
  (void)pvParameters;

  // Inisialisasi LCD
  lcd.begin(16, 2);
  lcd.backlight();

  while (1) {
    // Menampilkan pesan di LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Hello, I2C!");

    vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay 1 detik
  }
}

void setup() {
  // Inisialisasi komunikasi I2C
  Wire.begin();

  // Inisialisasi FreeRTOS task
  xTaskCreate(lcdTask, "lcdTask", 1000, NULL, 1, NULL);

  // Start FreeRTOS scheduler
  vTaskStartScheduler();
}

void loop() {
  // Tidak ada instruksi yang diperlukan di sini karena semua tugas dihandle oleh FreeRTOS
}