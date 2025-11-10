#include <Arduino.h>

#define ENC1_CLK 6
#define ENC1_DT  7

#define ENC2_CLK 15
#define ENC2_DT  16

volatile int encoder1Pos = 0;
volatile int encoder2Pos = 0;

int lastState1;
int lastState2;

TaskHandle_t taskEnc1;
TaskHandle_t taskEnc2;

void encoder1Task(void *param) {
  pinMode(ENC1_CLK, INPUT);
  pinMode(ENC1_DT, INPUT);
  lastState1 = digitalRead(ENC1_CLK);

  for (;;) {
    int currentState = digitalRead(ENC1_CLK);
    if (currentState != lastState1) {
      if (digitalRead(ENC1_DT) != currentState) encoder1Pos++;
      else encoder1Pos--;
      Serial.printf("[Core0] Encoder 1: %d\n", encoder1Pos);
      lastState1 = currentState;
    }
    vTaskDelay(2);
  }
}

void encoder2Task(void *param) {
  pinMode(ENC2_CLK, INPUT);
  pinMode(ENC2_DT, INPUT);
  lastState2 = digitalRead(ENC2_CLK);

  for (;;) {
    int currentState = digitalRead(ENC2_CLK);
    if (currentState != lastState2) {
      if (digitalRead(ENC2_DT) != currentState) encoder2Pos++;
      else encoder2Pos--;
      Serial.printf("[Core1] Encoder 2: %d\n", encoder2Pos);
      lastState2 = currentState;
    }
    vTaskDelay(2);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting Dual Rotary Encoder RTOS demo...");

 
  xTaskCreatePinnedToCore(encoder1Task, "Encoder1Task", 4096, NULL, 1, &taskEnc1, 0);
  xTaskCreatePinnedToCore(encoder2Task, "Encoder2Task", 4096, NULL, 1, &taskEnc2, 1);
}

void loop() {

}
