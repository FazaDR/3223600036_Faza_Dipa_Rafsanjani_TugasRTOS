//this will cause error in run time
//its intentional to make a point that esp32 cant read 2 adc at once (not counting wifi)
//and also adc here are nonthread safe
#include <Arduino.h>

#define POT1_PIN  1
#define POT2_PIN  2

TaskHandle_t potTask1;
TaskHandle_t potTask2;

void pot1Task(void *parameter) {
  for (;;) {
    int potValue = analogRead(POT1_PIN);
    float voltage = potValue * (3.3 / 4095.0);
    Serial.printf("[Core0] Pot1: %4d (%.2f V)\n", potValue, voltage);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void pot2Task(void *parameter) {
  for (;;) {
    int potValue = analogRead(POT2_PIN);
    float voltage = potValue * (3.3 / 4095.0);
    Serial.printf("[Core1] Pot2: %4d (%.2f V)\n", potValue, voltage);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Starting dual-core potentiometer demo...");

  pinMode(POT1_PIN, INPUT);
  pinMode(POT2_PIN, INPUT);

  xTaskCreatePinnedToCore(pot1Task, "PotTask1", 2048, NULL, 1, &potTask1, 0);
  xTaskCreatePinnedToCore(pot2Task, "PotTask2", 2048, NULL, 1, &potTask2, 1);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
