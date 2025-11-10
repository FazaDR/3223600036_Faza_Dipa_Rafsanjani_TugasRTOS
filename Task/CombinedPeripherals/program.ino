#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <type_traits>

// ---------------- Pin map ----------------
constexpr uint8_t PIN_LED1 = 5;
constexpr uint8_t PIN_LED2 = 6;

constexpr uint8_t PIN_BTN1 = 13;
constexpr uint8_t PIN_BTN2 = 14;

constexpr uint8_t PIN_ENC_CLK = 8;
constexpr uint8_t PIN_ENC_DT  = 9;
constexpr uint8_t PIN_ENC_SW  = 10;

constexpr uint8_t PIN_POT = 1;

constexpr uint8_t PIN_SERVO = 16;

constexpr uint8_t PIN_I2C_SDA = 17;
constexpr uint8_t PIN_I2C_SCL = 18;

constexpr uint8_t PIN_STEPPER_STEP = 21;
constexpr uint8_t PIN_STEPPER_DIR  = 35;

// ---------------- Display ----------------
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// ---------------- Servo ----------------
Servo servo;

// ---------------- Encoder ISR ----------------
volatile int32_t encAccum = 0;
volatile uint8_t lastA = 0;
volatile uint8_t lastB = 0;

IRAM_ATTR void isrEncA() {
  uint8_t a = digitalRead(PIN_ENC_CLK);
  uint8_t b = digitalRead(PIN_ENC_DT);
  if (a != lastA) {
    encAccum += (a == b) ? +1 : -1;
    lastA = a;
    lastB = b;
  }
}

// ---------------- Shared State ----------------
struct SystemState {
  int potRaw = 0;
  int servoAngle = 90;
  bool stepperEnabled = false;
  bool dirCW = true;
  int32_t target = 0;
  int32_t pos = 0;
  uint16_t stepHz = 800;
  bool ledHeartbeat = false;
  bool ledEnabled   = false;
  bool ledMoving    = false;
};

SystemState state;
SemaphoreHandle_t stateMtx;

// Helper that works for void and non-void lambdas
template<typename F>
auto withState(F&& f) {
  xSemaphoreTake(stateMtx, portMAX_DELAY);
  if constexpr (std::is_void_v<decltype(f(state))>) {
    f(state);
    xSemaphoreGive(stateMtx);
  } else {
    auto r = f(state);
    xSemaphoreGive(stateMtx);
    return r;
  }
}

// ---------------- Tasks ----------------
void TaskInput(void*) {
  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT,  INPUT_PULLUP);

  lastA = digitalRead(PIN_ENC_CLK);
  lastB = digitalRead(PIN_ENC_DT);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), isrEncA, CHANGE);

  bool prevBtn1 = true, prevBtn2 = true, prevEncSW = true;
  uint32_t potFilt = analogRead(PIN_POT);
  TickType_t lastBeat = xTaskGetTickCount();

  for (;;) {
    bool b1 = digitalRead(PIN_BTN1);
    bool b2 = digitalRead(PIN_BTN2);
    bool esw = digitalRead(PIN_ENC_SW);

    if (b1 != prevBtn1 && b1 == LOW) {
      withState([](SystemState& s){
        s.stepperEnabled = !s.stepperEnabled;
        s.ledEnabled = s.stepperEnabled;
        if (!s.stepperEnabled) s.ledMoving = false;
      });
    }
    prevBtn1 = b1;

    if (b2 != prevBtn2 && b2 == LOW) {
      withState([](SystemState& s){ s.dirCW = !s.dirCW; });
    }
    prevBtn2 = b2;

    if (esw != prevEncSW && esw == LOW) {
      withState([](SystemState& s){ s.target = 0; });
    }
    prevEncSW = esw;

    // Encoder
    int32_t delta = 0;
    noInterrupts();
    delta = encAccum;
    encAccum = 0;
    interrupts();
    if (delta) {
      withState([&](SystemState& s){ s.target += delta * 10; });
    }

    // Pot smoothing
    uint32_t raw = analogRead(PIN_POT);
    potFilt = (potFilt * 7 + raw) / 8;
    int angle = map(potFilt, 0, 4095, 0, 180);
    withState([&](SystemState& s){
      s.potRaw = potFilt;
      s.servoAngle = angle;
    });

    // Heartbeat
    if (xTaskGetTickCount() - lastBeat >= pdMS_TO_TICKS(500)) {
      withState([](SystemState& s){ s.ledHeartbeat = !s.ledHeartbeat; });
      lastBeat = xTaskGetTickCount();
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void TaskServo(void*) {
  servo.setPeriodHertz(50);
  servo.attach(PIN_SERVO, 500, 2500);
  for (;;) {
    int ang = withState([](SystemState& s){ return s.servoAngle; });
    servo.write(ang);
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void TaskStepper(void*) {
  pinMode(PIN_STEPPER_STEP, OUTPUT);
  pinMode(PIN_STEPPER_DIR, OUTPUT);
  digitalWrite(PIN_STEPPER_STEP, LOW);

  for (;;) {
    bool enabled, prefCW;
    int32_t tgt, pos;
    uint16_t hz;
    withState([&](SystemState& s){
      enabled = s.stepperEnabled;
      prefCW  = s.dirCW;
      tgt     = s.target;
      pos     = s.pos;
      hz      = s.stepHz;
    });

    if (!enabled) {
      withState([](SystemState& s){ s.ledMoving = false; });
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }

    int32_t diff = tgt - pos;
    if (diff == 0) {
      withState([](SystemState& s){ s.ledMoving = false; });
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    bool needPositive = diff > 0;
    bool physicalCW = needPositive ? prefCW : !prefCW;
    digitalWrite(PIN_STEPPER_DIR, physicalCW ? HIGH : LOW);

    uint32_t us = 1000000UL / (hz ? hz : 1);
    withState([](SystemState& s){ s.ledMoving = true; });

    digitalWrite(PIN_STEPPER_STEP, HIGH);
    delayMicroseconds(us / 2);
    digitalWrite(PIN_STEPPER_STEP, LOW);
    delayMicroseconds(us / 2);

    withState([&](SystemState& s){
      s.pos += needPositive ? +1 : -1;
    });
  }
}

void TaskLEDs(void*) {
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);

  for (;;) {
    SystemState snap = withState([](SystemState& s){ return s; });
    digitalWrite(PIN_LED1, snap.ledHeartbeat ? HIGH : LOW);
    digitalWrite(PIN_LED2, snap.ledEnabled   ? HIGH : LOW);
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void TaskDisplay(void*) {
  for (;;) {
    SystemState snap = withState([](SystemState& s){ return s; });

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.print(F("ESP32-S3 RTOS Demo"));

    display.setCursor(0, 12);
    display.printf("Pot:%4d Servo:%3d", snap.potRaw, snap.servoAngle);

    display.setCursor(0, 24);
    display.printf("Tgt:%ld", (long)snap.target);

    display.setCursor(0, 36);
    display.printf("Pos:%ld", (long)snap.pos);

    display.setCursor(0, 48);
    display.printf("Run:%s Dir:%s",
                   snap.stepperEnabled ? "YES" : "NO ",
                   snap.dirCW ? "CW " : "CCW");

    display.display();
    vTaskDelay(pdMS_TO_TICKS(120));
  }
}

void setup() {
  Serial.begin(115200);

  stateMtx = xSemaphoreCreateMutex();

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;) {
      Serial.println("SSD1306 init failed");
      delay(1000);
    }
  }

  xTaskCreatePinnedToCore(TaskInput,   "Input",   4096, nullptr, 3, nullptr, 1);
  xTaskCreatePinnedToCore(TaskServo,   "Servo",   3072, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(TaskStepper, "Stepper", 4096, nullptr, 3, nullptr, 0);
  xTaskCreatePinnedToCore(TaskLEDs,    "LEDs",    2048, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(TaskDisplay, "Display", 4096, nullptr, 1, nullptr, 1);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
