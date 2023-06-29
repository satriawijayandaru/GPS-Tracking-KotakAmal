/*
 * @AUTHOR .. SATRIA WIJAYANDARU - @yanda1998
 * 
 * PIN DECLARATION
 * # ESP8266 - PCB PROTOTYPE V0.1
 * - RESET BTN    = 0
 * - RFID SS PIN  = 5
 * - RFID RST PIN = 4
 * - RELAY SOLENOID = 10
 */

//#include <ESP8266WiFi.h>
//#include <ESP8266mDNS.h>
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
#define SS_PIN 5
#define RST_PIN 4
MFRC522 mfrc522(SS_PIN, RST_PIN);
String readUIDcontent;
bool UIDbtnStat = 0;
String compareUID;

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

//RELAY SOLENOID DECLARATION
int solenoidPin = 10;

void setup() {
  Serial.begin(115200);
  setupWifi();
  pinMode(btn, INPUT);
  pinMode(solenoidPin, OUTPUT);
  btn1.debounceTime   = 20;
  btn1.multiclickTime = 250;
  btn1.longClickTime = 2000;
  EEPROM.begin(512);
  SPI.begin();
  mfrc522.PCD_Init();
  compareUID = readEEPROM(rfidUID1);
  digitalWrite(solenoidPin, LOW);
}

void loop() {
  readRFID();
  readRFIDStandby();
  btn1.Update();
  if (btn1.clicks != 0) btnFunc = btn1.clicks;
  if (btnFunc == -1) {
    btnFunc = 0;
    UIDbtnStat = 1;
    Serial.println("Ready to read RFID");
    //    eepromClear();/
    Serial.print("EEPROM VALUE BEFORE = ");
    Serial.println(readEEPROM(rfidUID1));

  }
}

void solenoidControl(String UID){
  if(compareUID == UID){
    Serial.println("UNLOCKING SOLENOID");
    digitalWrite(solenoidPin, HIGH);
    delay(5000);
    Serial.println("LOCKING SOLENOID");
  }
  else{
    Serial.println("RFID NOT RECOGNIZED");
  }
}

void readRFID() {
  if (UIDbtnStat == 1) {
    if ( ! mfrc522.PICC_IsNewCardPresent()) {
      return;
    }
    if ( ! mfrc522.PICC_ReadCardSerial()) {
      return;
    }
    Serial.print(" UID tag :");

    String content = "";
    byte letter;
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    readUIDcontent = content;
    Serial.println(readUIDcontent);
    writeEEPROM(rfidUID1, readUIDcontent);
    Serial.print("EEPROM VALUE AFTER = ");
    Serial.println(readEEPROM(rfidUID1));
    compareUID = readEEPROM(rfidUID1);
    Serial.print("compareUID = ");
    Serial.println(compareUID);
    UIDbtnStat = 0;
  }
}

void readRFIDStandby() {
  if (UIDbtnStat == 0) {
    if ( ! mfrc522.PICC_IsNewCardPresent()) {
      return;
    }
    if ( ! mfrc522.PICC_ReadCardSerial()) {
      return;
    }
    Serial.print(" UID tag :");

    String content = "";
    byte letter;
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    readUIDcontent = content;
    Serial.println(readUIDcontent);
    solenoidControl(readUIDcontent);
    
  }
}

void writeEEPROM(int address, const String& data) {
  int length = data.length();

  // Write the length of the string as the first byte
  EEPROM.write(address, length);

  // Write each character of the string
  for (int i = 0; i < length; i++) {
    EEPROM.write(address + 1 + i, data[i]);
  }

  // Commit the changes to EEPROM
  EEPROM.commit();
}

String readEEPROM(int address) {
  int length = EEPROM.read(address); // Read the length of the string

  String data = "";
  for (int i = 0; i < length; i++) {
    char character = EEPROM.read(address + 1 + i); // Read each character of the string
    data += character;
  }

  return data;
}

void eepromClear() {
  Serial.println("Clearing EEPROM");
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
    Serial.println(i);
  }
  EEPROM.commit();
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
