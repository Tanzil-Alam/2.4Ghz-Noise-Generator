//The Librarys
#include "RF24.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "esp_bt.h"
#include "esp_wifi.h"
#include <Arduino.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
// The oled configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// fixed Spi for both nrf
#define NRF_SCK   5
#define NRF_MOSI  6
#define NRF_MISO  7

#define PIN_SELECT 2

SPIClass *sp = nullptr;
SPIClass *hp = nullptr;
RF24 radio(3, 4, 16000000);
RF24 radio1(0, 1, 16000000);

bool radio1OK = false, radio2OK = false;
unsigned int flag = 0, flagv = 0;
int ch = 45, ch1 = 45;

void two() {
  if (flagv == 0) ch1 += 4; else ch1 -= 4;
  if (flag == 0) ch += 2; else ch -= 2;
  if ((ch1 > 79) && (flagv == 0)) flagv = 1;
  else if ((ch1 < 2) && (flagv == 1)) flagv = 0;
  if ((ch > 79) && (flag == 0)) flag = 1;
  else if ((ch < 2) && (flag == 1)) flag = 0;
  radio.setChannel(ch);
  radio1.setChannel(ch1);
}

void one() {
  radio1.setChannel(random(80));
  radio.setChannel(random(80));
  delayMicroseconds(random(60));
}

void initSP() {
  sp = &SPI;
  if (radio1.begin(sp)) {
    Serial.println("VSPI Jammer Started !!!");
    radio1.setAutoAck(false);
    radio1.stopListening();
    radio1.setRetries(0, 0);
    radio1.setPALevel(RF24_PA_MAX, true);
    radio1.setDataRate(RF24_2MBPS);
    radio1.setCRCLength(RF24_CRC_DISABLED);
    radio1.printPrettyDetails();
    radio1.startConstCarrier(RF24_PA_MAX, ch1);
    radio2OK = true;
  } else {
    Serial.println("VSPI Jammer couldn't start !!!");
    radio2OK = false;
  }
}

void initHP() {
  hp = &SPI;
  if (radio.begin(hp)) {
    Serial.println("HSPI Started !!!");
    radio.setAutoAck(false);
    radio.stopListening();
    radio.setRetries(0, 0);
    radio.setPALevel(RF24_PA_MAX, true);
    radio.setDataRate(RF24_2MBPS);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.printPrettyDetails();
    radio.startConstCarrier(RF24_PA_MAX, ch);
    radio1OK = true;
  } else {
    Serial.println("HSPI couldn't start !!!");
    radio1OK = false;
  }
}

void runPureJammingLoop() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  int runW = 10 * 12;
  display.setCursor((128 - runW) / 2, 24);
  display.print("RUNNING...");
  display.display();

  radio.powerUp();
  radio1.powerUp();
  delay(15);
  initHP();
  initSP();

  while (digitalRead(PIN_SELECT) == LOW) delay(10);
  delay(250);

  // Jamming loop – exit when Select is pressed OR serial 'S' received
  while (true) {
    if (digitalRead(PIN_SELECT) == LOW) break;
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'S' || c == 's') {
        Serial.println("Serial stop command received");
        break;
      }
    }
    one();
  }

  radio.stopConstCarrier();
  radio1.stopConstCarrier();

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  int stopW = 7 * 12;
  display.setCursor((128 - stopW) / 2, 24);
  display.print("Stopped");
  display.display();

  while (digitalRead(PIN_SELECT) == LOW) delay(10);
  delay(800);
}

void setup() {
  Serial.begin(115200);
  Serial.println("=== RF Jammer Ready ===");
  Serial.println("Type S to stop jamming. U and D are just echoes.");

  Wire.begin(8, 9);
  Wire.setClock(400000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
  }

  esp_bt_controller_deinit();
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_wifi_disconnect();

  SPI.begin(NRF_SCK, NRF_MISO, NRF_MOSI);
  initHP();
  initSP();

  runPureJammingLoop();
}

void loop() {
  delay(2000);
  runPureJammingLoop();
}