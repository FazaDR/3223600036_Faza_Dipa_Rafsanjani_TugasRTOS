#include <Arduino.h>
#include <ESP32Servo.h>

#define SERVO1_PIN 16
#define SERVO2_PIN 17

Servo servo1;
Servo servo2;

TaskHandle_t taskServo1;
TaskHandle_t taskServo2;

void servo1Task(void *parameter) {
  int angle = 45;
  int dir = 1;
  for (;;) {
    servo1.write(angle);
    angle += dir * 2;
    if (angle >= 135 || angle <= 45) dir = -dir;
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void servo2Task(void *parameter) {
  int angle = 45;
  int dir = 1;
  for (;;) {
    servo2.write(angle);
    angle += dir * 2;
    if (angle >= 135 || angle <= 45) dir = -dir;
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Starting dual-core servo sync test...");

  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);

  servo1.write(45);
  servo2.write(45);

  xTaskCreatePinnedToCore(servo1Task, "Servo1 Task", 2048, NULL, 1, &taskServo1, 0);
  xTaskCreatePinnedToCore(servo2Task, "Servo2 Task", 2048, NULL, 1, &taskServo2, 1);
}

void loop() {
}
