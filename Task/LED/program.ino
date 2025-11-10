#include <Arduino.h>


#define LED_CORE0  12
#define LED_CORE1  13

TaskHandle_t taskLED0;
TaskHandle_t taskLED1;

void ledCore0Task(void *parameter) {
  pinMode(LED_CORE0, OUTPUT);
  while (true) {
    digitalWrite(LED_CORE0, HIGH);
    vTaskDelay(pdMS_TO_TICKS(500));
    digitalWrite(LED_CORE0, LOW);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void ledCore1Task(void *parameter) {
  pinMode(LED_CORE1, OUTPUT);
  while (true) {
    digitalWrite(LED_CORE1, HIGH);
    vTaskDelay(pdMS_TO_TICKS(300));
    digitalWrite(LED_CORE1, LOW);
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Starting LED tasks on different cores...");

  xTaskCreatePinnedToCore(ledCore0Task, "LED_Core0_Task", 2048, NULL, 1, &taskLED0, 0);
  xTaskCreatePinnedToCore(ledCore1Task, "LED_Core1_Task", 2048, NULL, 1, &taskLED1, 1);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
