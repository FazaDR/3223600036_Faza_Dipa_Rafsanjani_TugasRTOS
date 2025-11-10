#include <Arduino.h>

#define BUZZER1_PIN  18
#define BUZZER2_PIN  37

TaskHandle_t buzzerTask1;
TaskHandle_t buzzerTask2;

void buzzer1Task(void *parameter) {
  pinMode(BUZZER1_PIN, OUTPUT);
  while (true) {
    tone(BUZZER1_PIN, 1000);
    vTaskDelay(pdMS_TO_TICKS(200));
    noTone(BUZZER1_PIN);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void buzzer2Task(void *parameter) {
  pinMode(BUZZER2_PIN, OUTPUT);
  while (true) {
    tone(BUZZER2_PIN, 1500);
    vTaskDelay(pdMS_TO_TICKS(300));
    noTone(BUZZER2_PIN);
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Starting dual-core buzzer demo...");
  xTaskCreatePinnedToCore(buzzer1Task, "BuzzerCore0", 2048, NULL, 1, &buzzerTask1, 0);
  xTaskCreatePinnedToCore(buzzer2Task, "BuzzerCore1", 2048, NULL, 1, &buzzerTask2, 1);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
