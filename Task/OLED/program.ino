#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define I2C_SDA 17
#define I2C_SCL 18

#define OLED_ADDR_1 0x3C
#define OLED_ADDR_2 0x3D

Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

TaskHandle_t taskHandleDisp1 = NULL;
TaskHandle_t taskHandleDisp2 = NULL;

void taskDisplay1(void* pvParameters);
void taskDisplay2(void* pvParameters);

void setup() {
  Serial.begin(115200);
  delay(100);

  Wire.begin(I2C_SDA, I2C_SCL);

  if (!display1.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR_1)) {
    Serial.println("SSD1306 allocation failed for display1 (0x3C).");
  } else {
    display1.clearDisplay();
    display1.setTextSize(1);
    display1.setTextColor(SSD1306_WHITE);
    display1.setCursor(0, 0);
    display1.println("Display 1 (0x3C) ready");
    display1.display();
  }

  if (!display2.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR_2)) {
    Serial.println("SSD1306 allocation failed for display2 (0x3D).");
  } else {
    display2.clearDisplay();
    display2.setTextSize(1);
    display2.setTextColor(SSD1306_WHITE);
    display2.setCursor(0, 0);
    display2.println("Display 2 (0x3D) ready");
    display2.display();
  }

  BaseType_t r1 = xTaskCreatePinnedToCore(taskDisplay1, "Display1", 4096, NULL, 1, &taskHandleDisp1, 0);
  BaseType_t r2 = xTaskCreatePinnedToCore(taskDisplay2, "Display2", 4096, NULL, 1, &taskHandleDisp2, 1);

  Serial.printf("Task create results: disp1=%d disp2=%d\n", (int)r1, (int)r2);
}

void loop() {
  delay(1000);
}

// Task for Display 1: Counter + Moving Bar
void taskDisplay1(void* pvParameters) {
  uint32_t counter = 0;
  int pos = 0;
  int dir = 1;

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(100));
    display1.clearDisplay();

    display1.setTextSize(2);
    display1.setTextColor(SSD1306_WHITE);
    display1.setCursor(0, 0);
    display1.printf("A:%04u", counter & 0xFFFF);

    int y = 48;
    display1.drawRect(0, y, SCREEN_WIDTH, 12, SSD1306_WHITE);
    display1.fillRect(pos, y + 2, 10, 8, SSD1306_WHITE);
    display1.display();

    counter++;
    pos += dir * 2;
    if (pos <= 0) { pos = 0; dir = 1; }
    if (pos >= SCREEN_WIDTH - 10) { pos = SCREEN_WIDTH - 10; dir = -1; }
  }
  vTaskDelete(NULL);
}

// Task for Display 2: Rotating Dot Animation
void taskDisplay2(void* pvParameters) {
  uint32_t counter = 0;
  float angle = 0.0;

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(140));
    display2.clearDisplay();

    display2.setTextSize(1);
    display2.setTextColor(SSD1306_WHITE);
    display2.setCursor(0, 0);
    display2.println("Display 2 (core 1)");
    display2.printf("Tick: %u\n", counter);

    int cx = SCREEN_WIDTH / 2;
    int cy = 42;
    int r = 12;
    int x = cx + int(r * cos(angle));
    int y = cy + int(r * sin(angle));
    display2.drawCircle(cx, cy, r + 2, SSD1306_WHITE);
    display2.fillCircle(x, y, 3, SSD1306_WHITE);
    display2.display();

    counter++;
    angle += 0.35f;
    if (angle > 2.0f * PI) angle -= 2.0f * PI;
  }
  vTaskDelete(NULL);
}
