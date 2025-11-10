#include <Arduino.h>

#define BTN1_PIN 13 
#define BTN2_PIN 14 

TaskHandle_t taskBtn1;
TaskHandle_t taskBtn2;

volatile bool stateBtn1 = false;
volatile bool stateBtn2 = false;

void btn1Task(void *param) {
  pinMode(BTN1_PIN, INPUT_PULLUP);
  bool lastState = HIGH;

  for (;;) {
    bool current = digitalRead(BTN1_PIN);
    if (current == LOW && lastState == HIGH) { 
      stateBtn1 = !stateBtn1;
      Serial.printf("[Core0] Button 1 toggled -> %d\n", stateBtn1);
      vTaskDelay(pdMS_TO_TICKS(250));
    }
    lastState = current;
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void btn2Task(void *param) {
  pinMode(BTN2_PIN, INPUT_PULLUP);
  bool lastState = HIGH;

  for (;;) {
    bool current = digitalRead(BTN2_PIN);
    if (current == LOW && lastState == HIGH) {
      stateBtn2 = !stateBtn2;
      Serial.printf("[Core1] Button 2 toggled -> %d\n", stateBtn2);
      vTaskDelay(pdMS_TO_TICKS(250));
    }
    lastState = current;
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Starting Dual-Core Button Toggle Demo...");

  xTaskCreatePinnedToCore(btn1Task, "Button1Task", 2048, NULL, 1, &taskBtn1, 0);
  xTaskCreatePinnedToCore(btn2Task, "Button2Task", 2048, NULL, 1, &taskBtn2, 1);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
