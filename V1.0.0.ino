#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Adafruit_SSD1306.h> // Instal library OLED dari Arduino Library Manager
#include <Adafruit_GFX.h> // Dependensi dari library OLED

SemaphoreHandle_t mutex_v;
SemaphoreHandle_t soilMoistureSemaphore;
TaskHandle_t HandleMoisture;
TaskHandle_t HandleWaterPump;
TaskHandle_t HandleOLED;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int soilMoisturePin = A0;
const int waterPumpPin = 9;

void TaskMoisture(void *pvParameters);
void TaskWaterPump(void *pvParameters);
void TaskOLED(void *pvParameters);

void setup() {
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D); // Inisialisasi OLED

  mutex_v = xSemaphoreCreateMutex();
  soilMoistureSemaphore = xSemaphoreCreateBinary();

  if (mutex_v == NULL || soilMoistureSemaphore == NULL) {
    Serial.println("Semaphore creation failed.");
  }

  xTaskCreate(TaskMoisture, "Moisture", 128, NULL, 2, &HandleMoisture);
  xTaskCreate(TaskWaterPump, "WaterPump", 128, NULL, 1, &HandleWaterPump);
  xTaskCreate(TaskOLED, "OLED", 128, NULL, 1, &HandleOLED);

  pinMode(waterPumpPin, OUTPUT);
  digitalWrite(waterPumpPin, LOW);
  vTaskStartScheduler();
}

void loop() {}

void TaskMoisture(void *pvParameters) {
  (void)pvParameters;
  while (1) {
    int moistureLevel = analogRead(soilMoisturePin);
    if (moistureLevel < 500) {
      xSemaphoreGive(soilMoistureSemaphore); // Aktifkan semaphore jika kelembaban kurang dari ambang tertentu
    } else {
      xSemaphoreTake(soilMoistureSemaphore, portMAX_DELAY); // Tunggu semaphore jika kelembaban mencukupi
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void TaskWaterPump(void *pvParameters) {
  (void)pvParameters;
  while (1) {
    if (xSemaphoreTake(soilMoistureSemaphore, portMAX_DELAY) == pdTRUE) {
      xSemaphoreTake(mutex_v, portMAX_DELAY);
      digitalWrite(waterPumpPin, HIGH); // Nyalakan water pump
      vTaskDelay(5000 / portTICK_PERIOD_MS); // Pompa air selama 5 detik
      digitalWrite(waterPumpPin, LOW); // Matikan water pump
      xSemaphoreGive(mutex_v);
    }
  }
}

void TaskOLED(void *pvParameters) {
  (void)pvParameters;
  display.display(); // Inisialisasi tampilan OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  while (1) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Soil Moisture: ");
    int moistureLevel = analogRead(soilMoisturePin);
    display.print(moistureLevel);

    display.display();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}