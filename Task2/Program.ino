#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

constexpr uint8_t PIN_LED1 = 5;
constexpr uint8_t PIN_LED2 = 6;
constexpr uint8_t PIN_LED3 = 7;

constexpr uint8_t PIN_BTN1 = 13;
constexpr uint8_t PIN_BTN2 = 14;

constexpr uint8_t PIN_POT  = 1;

constexpr uint8_t PIN_ENC_CLK = 8;
constexpr uint8_t PIN_ENC_DT  = 9;
constexpr uint8_t PIN_ENC_SW  = 10;

constexpr uint8_t PIN_SERVO = 16;

constexpr uint8_t PIN_STP_STEP = 21;
constexpr uint8_t PIN_STP_DIR  = 35;

constexpr uint8_t PIN_BUZZER = 37;

constexpr uint8_t PIN_I2C_SDA = 17;
constexpr uint8_t PIN_I2C_SCL = 18;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Servo myServo;

TaskHandle_t thLED1, thLED2, thLED3;
TaskHandle_t thBTN1, thBTN2;
TaskHandle_t thPOT, thENC, thSERVO, thSTEPPER, thOLED, thBUZZ;

inline void servoPulse(uint8_t pin, uint16_t highUs) {
  if (highUs < 500)  highUs = 500;
  if (highUs > 2500) highUs = 2500;
  const uint32_t frameUs = 20000;
  digitalWrite(pin, HIGH);
  delayMicroseconds(highUs);
  digitalWrite(pin, LOW);
  delayMicroseconds(frameUs - highUs);
}

struct LedTaskParams {
  uint8_t pin;
  uint32_t intervalMs;
};

void taskBlinkLED(void* pv) {
  LedTaskParams p = *(LedTaskParams*)pv;
  pinMode(p.pin, OUTPUT);
  bool state = false;
  for (;;) {
    digitalWrite(p.pin, state);
    state = !state;
    vTaskDelay(pdMS_TO_TICKS(p.intervalMs));
  }
}

void taskButton(void* pv) {
  uint8_t pin = (uint32_t)pv;
  pinMode(pin, INPUT_PULLUP);
  int last = digitalRead(pin);
  for (;;) {
    int cur = digitalRead(pin);
    if (cur != last) {
      last = cur;
      Serial.printf("Button on GPIO %u: %s\n", pin, (cur == LOW) ? "PRESSED" : "RELEASED");
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void taskPot(void* pv) {
  analogReadResolution(12);
  for (;;) {
    int val = analogRead(PIN_POT);
    Serial.printf("POT ADC@GPIO %u = %d\n", PIN_POT, val);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void taskEncoder(void* pv) {
  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT,  INPUT_PULLUP);
  pinMode(PIN_ENC_SW,  INPUT_PULLUP);

  int32_t position = 0;
  int lastCLK = digitalRead(PIN_ENC_CLK);
  int lastSW = digitalRead(PIN_ENC_SW);

  for (;;) {
    // Rotary
    int clk = digitalRead(PIN_ENC_CLK);
    if (clk != lastCLK) {
      int dt = digitalRead(PIN_ENC_DT);
      if (dt != clk) position++;
      else position--;
      Serial.printf("Encoder pos = %ld\n", (long)position);
      lastCLK = clk;
    }
    // Switch
    int sw = digitalRead(PIN_ENC_SW);
    if (sw != lastSW) {
      lastSW = sw;
      if (sw == LOW) Serial.println("Encoder SW: PRESSED");
      else           Serial.println("Encoder SW: RELEASED");
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void taskServo(void* pv) {
  myServo.attach(PIN_SERVO, 500, 2500);
  int angle = 0;
  int dir = 1;
  for (;;) {
    myServo.write(angle);

    angle += dir;
    if (angle >= 180) { angle = 180; dir = -1; }
    if (angle <= 0)   { angle = 0;   dir = 1; }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void taskStepper(void* pv) {
  pinMode(PIN_STP_STEP, OUTPUT);
  pinMode(PIN_STP_DIR,  OUTPUT);

  const uint32_t stepDelayUs = 1000;
  const int stepsPerMove = 200;

  for (;;) {
    // Forward
    digitalWrite(PIN_STP_DIR, HIGH);
    for (int i = 0; i < stepsPerMove; ++i) {
      digitalWrite(PIN_STP_STEP, HIGH);
      delayMicroseconds(stepDelayUs);
      digitalWrite(PIN_STP_STEP, LOW);
      delayMicroseconds(stepDelayUs);
      if ((i % 40) == 0) taskYIELD();
    }
    vTaskDelay(pdMS_TO_TICKS(500));

    // Reverse
    digitalWrite(PIN_STP_DIR, LOW);
    for (int i = 0; i < stepsPerMove; ++i) {
      digitalWrite(PIN_STP_STEP, HIGH);
      delayMicroseconds(stepDelayUs);
      digitalWrite(PIN_STP_STEP, LOW);
      delayMicroseconds(stepDelayUs);
      if ((i % 40) == 0) taskYIELD();
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void taskOLED(void* pv) {
  uint32_t counter = 0;
  for (;;) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println("ESP32-S3 Peripherals");
    display.setCursor(0, 16);
    display.println("Running in FreeRTOS");
    display.setCursor(0, 32);
    display.print("OLED Task Count: ");
    display.println(counter++);
    display.setCursor(0, 48);
    display.print("Millis: ");
    display.println(millis());

    display.display();
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

void taskBuzzer(void* pv) {
  pinMode(PIN_BUZZER, OUTPUT);
  for (;;) {
    digitalWrite(PIN_BUZZER, HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    digitalWrite(PIN_BUZZER, LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("OLED initialized");
    display.display();
  }

  static LedTaskParams p1{PIN_LED1, 500};
  static LedTaskParams p2{PIN_LED2, 700};
  static LedTaskParams p3{PIN_LED3, 900};
  xTaskCreate(taskBlinkLED, "LED1", 2048, &p1, 1, &thLED1);
  xTaskCreate(taskBlinkLED, "LED2", 2048, &p2, 1, &thLED2);
  xTaskCreate(taskBlinkLED, "LED3", 2048, &p3, 1, &thLED3);
  xTaskCreate(taskButton, "BTN1", 2048, (void*)(uint32_t)PIN_BTN1, 1, &thBTN1);
  xTaskCreate(taskButton, "BTN2", 2048, (void*)(uint32_t)PIN_BTN2, 1, &thBTN2);
  xTaskCreate(taskPot, "POT", 2048, nullptr, 1, &thPOT);
  xTaskCreate(taskEncoder, "ENC", 3072, nullptr, 1, &thENC);
  xTaskCreate(taskServo, "SERVO", 2048, nullptr, 1, &thSERVO);
  xTaskCreate(taskStepper, "STEPPER", 3072, nullptr, 1, &thSTEPPER);
  xTaskCreate(taskOLED, "OLED", 4096, nullptr, 1, &thOLED);
  xTaskCreate(taskBuzzer, "BUZZ", 2048, nullptr, 1, &thBUZZ);
}

void loop() {
  // Nothing here. All work is done in tasks.
}