#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include "ClickButton.h"

//WIFI AND MQTT BROKER DECLARATION
#ifndef STASSID
#define STASSID "XDA"
#define STAPSK  "namaguee"
#endif
const char* ssid = STASSID;
const char* password = STAPSK;
const char* mqtt_server = "11.11.11.111";
const char* host = "kotak amal";
WiFiClient espClient;
PubSubClient client(espClient);
const char* ssidAp = "ESP8266 Hotspot";
const char* passwordAp = "password";
String selectedSSID;
String selectedPassword;
String storedSSID = "";
String storedPassword = "";

//RFID DECLARATION
#define SS_PIN 2
#define RST_PIN 0
MFRC522 mfrc522(SS_PIN, RST_PIN);

//BUTTON CONFIG (ON BOARD FLASH BUTTON)
int btn = 0;
int btnFunc = 0;
ClickButton btn1(btn, LOW, CLICKBTN_PULLUP);

//EEPROM MEMORY DECLARATION
const int EEPROM_SIZE = 512;
int wifi_ssid_address = 0;
int wifi_password_address = 32;
int rfidUID1 = 64;
int rfidUID2 = 96;

//LED DECLARATION
int ledPin = LED_BUILTIN;

void setup() {
  Serial.begin(115200);
  setupWifi();
  pinMode(btn, INPUT);
  btn1.debounceTime   = 20;
  btn1.multiclickTime = 250;
  btn1.longClickTime = 2000;
  EEPROM.begin(64);
}

void loop() {
  btn1.Update();
  if (btn1.clicks != 0) btnFunc = btn1.clicks;
  if (btnFunc == -1) {
    btnFunc = 0;
    Serial.println("Clearing EEPROM");
    for (int i = 0 ; i < EEPROM.length() ; i++) {
      EEPROM.write(i, 0);
      Serial.println(i);
    }
    EEPROM.commit();
  }
}

void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi Tersambung");
  Serial.println("Alamat IP MQTT Client : ");
  Serial.println(WiFi.localIP());
}
