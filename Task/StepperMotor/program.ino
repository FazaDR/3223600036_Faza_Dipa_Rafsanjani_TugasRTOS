#include <Arduino.h>

// --- Stepper 1 pins ---
#define STEP1_PIN 17
#define DIR1_PIN  35

#define STEP2_PIN 37
#define DIR2_PIN  36

// task handles
TaskHandle_t task1Handle = NULL;
TaskHandle_t task2Handle = NULL;

// parameters
const int pulseDelay = 800; // microseconds between step high/low
const int stepCount = 200;  // steps per revolution approx.

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(STEP1_PIN, OUTPUT);
  pinMode(DIR1_PIN, OUTPUT);
  pinMode(STEP2_PIN, OUTPUT);
  pinMode(DIR2_PIN, OUTPUT);

  digitalWrite(DIR1_PIN, HIGH);
  digitalWrite(DIR2_PIN, HIGH);

  Serial.println("Creating stepper tasks...");

  // create two tasks pinned to different cores
  xTaskCreatePinnedToCore(
    [](void*) {
      for (;;) {
        // spin forward
        digitalWrite(DIR1_PIN, HIGH);
        for (int i = 0; i < stepCount; i++) {
          digitalWrite(STEP1_PIN, HIGH);
          delayMicroseconds(pulseDelay);
          digitalWrite(STEP1_PIN, LOW);
          delayMicroseconds(pulseDelay);
        }
        // short pause
        vTaskDelay(pdMS_TO_TICKS(200));
      }
    },
    "Stepper1Task", 2048, NULL, 1, &task1Handle, 0
  );

  xTaskCreatePinnedToCore(
    [](void*) {
      for (;;) {
        digitalWrite(DIR2_PIN, HIGH);
        for (int i = 0; i < stepCount; i++) {
          digitalWrite(STEP2_PIN, HIGH);
          delayMicroseconds(pulseDelay);
          digitalWrite(STEP2_PIN, LOW);
          delayMicroseconds(pulseDelay);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
      }
    },
    "Stepper2Task", 2048, NULL, 1, &task2Handle, 1
  );
}

void loop() {
  // main loop does nothing, both steppers run under RTOS
  vTaskDelay(pdMS_TO_TICKS(1000));
}
