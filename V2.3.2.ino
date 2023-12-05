#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

SemaphoreHandle_t mutex_v;
SemaphoreHandle_t soilMoistureSemaphore;
SemaphoreHandle_t buttonSemaphore;
TaskHandle_t HandleMoisture;
TaskHandle_t HandleWaterPump;
QueueHandle_t moistureQueue;

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
  pinMode(buttonPin, INPUT_PULLUP);

  mutex_v = xSemaphoreCreateMutex();
  soilMoistureSemaphore = xSemaphoreCreateBinary();
  buttonSemaphore = xSemaphoreCreateBinary();
  moistureQueue = xQueueCreate(5, sizeof(int)); // Create a queue with a depth of 5

  if (mutex_v == NULL || soilMoistureSemaphore == NULL || buttonSemaphore == NULL || moistureQueue == NULL) {
    Serial.println("Semaphore/Queue creation failed.");
  }

  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonInterrupt, FALLING);

  xTaskCreate(TaskMoisture, "Moisture", 128, NULL, 2, &HandleMoisture);
  xTaskCreate(TaskWaterPump, "WaterPump", 128, NULL, 1, &HandleWaterPump);

  lcd.init();      // Initialize the LCD
  lcd.backlight(); // Turn on the backlight
}

void loop() {}

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

    // Send moisture level to the queue
    xQueueSend(moistureQueue, &moistureLevel, portMAX_DELAY);

    if (moistureLevel > 500) {
      xSemaphoreGive(soilMoistureSemaphore);
      changeTaskPriority(HandleWaterPump, 3);
      Serial.println("Dibawah");
    } else {
      changeTaskPriority(HandleWaterPump, 1); // Add alternative behavior for the else condition
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void changeTaskPriority(TaskHandle_t taskHandle, UBaseType_t newPriority) {
  vTaskPrioritySet(taskHandle, newPriority);
}

void TaskWaterPump(void *pvParameters) {
  (void)pvParameters;
  pinMode(waterPumpPin, OUTPUT);

  while (1) {
    int moistureLevel;
    if (xQueueReceive(moistureQueue, &moistureLevel, portMAX_DELAY) == pdTRUE) {
      if (moistureLevel > 500) {
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
}

void buttonInterrupt() {
  xSemaphoreGive(soilMoistureSemaphore);
  Serial.println("Butt");
}
