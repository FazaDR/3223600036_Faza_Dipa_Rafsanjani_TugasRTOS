#include <Arduino.h>

#define LED1 6
#define LED2 1

TaskHandle_t task1Handle;
TaskHandle_t task2Handle;

void task1(void *param) {
  pinMode(LED1, OUTPUT);
  while (true) {
    digitalWrite(LED1, HIGH);
    digitalWrite(LED1, LOW);
    Serial.println("Task1Run");
  }
}

void task2(void *param) {
  pinMode(LED2, OUTPUT);
  while (true) {
    digitalWrite(LED2, HIGH);
    digitalWrite(LED1, LOW);
    Serial.println("Task2Run");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting task priority test...");
  xTaskCreatePinnedToCore(task1, "Task1", 2048, NULL, 1, &task1Handle, 1);
  xTaskCreatePinnedToCore(task2, "Task2", 2048, NULL, 2, &task2Handle, 1);
}

void loop() {
}
