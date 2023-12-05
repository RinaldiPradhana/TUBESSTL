#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

SemaphoreHandle_t mutex_v;
SemaphoreHandle_t soilMoistureSemaphore;
SemaphoreHandle_t buttonSemaphore;
TaskHandle_t HandleMoisture;
TaskHandle_t HandleWaterPump;

const int soilMoisturePin = A0;
const int waterPumpPin = 9;
const int buttonPin = 2;

LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the I2C address and LCD dimensions

int getMoistureLevel() {
  int moistureLevel;
  portENTER_CRITICAL();
  moistureLevel = analogRead(soilMoisturePin);
  portEXIT_CRITICAL();
  return moistureLevel;
}

void TaskMoisture(void *pvParameters);
void TaskWaterPump(void *pvParameters);

void buttonInterrupt();

void setup() {
  Serial.begin(9600);

  mutex_v = xSemaphoreCreateMutex();
  soilMoistureSemaphore = xSemaphoreCreateBinary();
  buttonSemaphore = xSemaphoreCreateBinary();

  if (mutex_v == NULL || soilMoistureSemaphore == NULL || buttonSemaphore == NULL) {
    Serial.println("Semaphore creation failed.");
  }

  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonInterrupt, RISING);

  xTaskCreate(TaskMoisture, "Moisture", 128, NULL, 2, &HandleMoisture);
  xTaskCreate(TaskWaterPump, "WaterPump", 128, NULL, 1, &HandleWaterPump);

  lcd.init();      // Initialize the LCD
  lcd.backlight(); // Turn on the backlight
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay dalam loop utama
}

void TaskMoisture(void *pvParameters) {
  (void)pvParameters;
  while (1) {
    int moistureLevel = getMoistureLevel();
    Serial.println(moistureLevel);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Soil Moisture:");
    lcd.setCursor(5, 1);
    lcd.print(moistureLevel);
    if (moistureLevel > 500) {
      xSemaphoreGive(soilMoistureSemaphore);
      Serial.println("Dibawah");
    } else {
      // Tambahkan perilaku alternatif untuk kondisi else
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void TaskWaterPump(void *pvParameters) {
  (void)pvParameters;
  pinMode(waterPumpPin, OUTPUT);

  while (1) {
    if (xSemaphoreTake(soilMoistureSemaphore, portMAX_DELAY) == pdTRUE || xSemaphoreTake(buttonSemaphore, portMAX_DELAY) == pdTRUE) {
      xSemaphoreTake(mutex_v, portMAX_DELAY);
      digitalWrite(waterPumpPin, HIGH);
      Serial.println("Nyala");
      vTaskDelay(5000 / portTICK_PERIOD_MS);
      digitalWrite(waterPumpPin, LOW);
      Serial.println("Mati");
      xSemaphoreGive(mutex_v);
    }
  }
}

void buttonInterrupt() {
  xSemaphoreGiveFromISR(buttonSemaphore, NULL);
  Serial.println("Butt");
  portYIELD_FROM_ISR();
}
