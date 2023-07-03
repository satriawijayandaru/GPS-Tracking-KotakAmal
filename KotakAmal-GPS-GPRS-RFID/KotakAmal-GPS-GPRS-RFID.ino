/*
   @AUTHOR .. SATRIA WIJAYANDARU - @yanda1998

   PIN DECLARATION
   # ESP8266 - PCB PROTOTYPE V0.1
   - RESET BTN    = 0
   - RFID SS PIN  = 5
   - RFID RST PIN = 4
   - RELAY SOLENOID = 21
   - BUZZER PIN   = 13
   - GPS RX       = 22
*/

/*
   ERROR CODE DECLARATION\
   0 = WORKING FINE
   1 = RFID MODULE NOT DETECTED
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
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include "SIM800L.h"

#define internalLed 2

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
bool isRC522Detected = false;

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
int solenoidPin = 21;

//SIM MODULE SIM800L DECLARATION
#define SIM800_RST_PIN 39             //CHANGE IF NECESSARY, THIS PIN IS INPUT ONLY
#define RXD2 16
#define TXD2 17
int simBaudRate = 9600;
SIM800L* sim800l;

//GPS DECLARATION
int gpsRX = 22;
int gpsTX = 36;                       //CHANGE IF NECESSARY, THIS PIN IS INPUT ONLY
double Lat, Lon, Alt;
//double lat, lng, alt/;
TinyGPSPlus gps;
SoftwareSerial gpsSerial(gpsRX, gpsTX);
//EspSoftwareSerial::UART gpsSerial;/

//BUZZER DECLARATION
int buzzerPin = 13;

//ERROR FLAG CODE
int errorCode = 0;
unsigned long previousMillisErrorHandling = 0;
const long intervalErrorHandling = 1000;

void setup() {
  Serial.begin(115200);
  Serial2.begin(simBaudRate, SERIAL_8N1, RXD2, TXD2);
  gpsSerial.begin(9600);
  //  setupWifi();/
  pinMode(btn, INPUT);
  pinMode(solenoidPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(internalLed, OUTPUT);

  sim800l = new SIM800L((Stream *)&Serial2, SIM800_RST_PIN, 200, 512);
  checkSIM();

  btn1.debounceTime   = 20;
  btn1.multiclickTime = 250;
  btn1.longClickTime = 2000;
  EEPROM.begin(512);
  SPI.begin();
  mfrc522.PCD_Init();
  mfrc522.PCD_DumpVersionToSerial();
  detectRC522Module();
  compareUID = readEEPROM(rfidUID1);
  digitalWrite(solenoidPin, LOW);
  //  checkGpsSerial();/
  startupTone();
}

void loop() {
  readRFID();
  readRFIDStandby();
  getGPSData();

  //  detectRC522Module();/
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
  unsigned long currentMillisErrorHandling = millis();
  if (currentMillisErrorHandling - previousMillisErrorHandling >= intervalErrorHandling) {
    previousMillisErrorHandling = currentMillisErrorHandling;
    //    mfrc522.PCD_DumpVersionToSerial(); /
    //    errorChecking();/
  }
}

void errorChecking() {
  Serial.print("RFID STATUS : ");
  Serial.println(detectRC522Module());
}

void checkSIM() {
  while (!sim800l->isReady()) {
    Serial.println(F("Problem to initialize AT command, retry in 1 sec"));
    delay(1000);
  }

  // Active echo mode (for some module, it is required)
  sim800l->enableEchoMode();

  Serial.println("Module ready");

  // Print version
  Serial.print("Module ");
  Serial.println(sim800l->getVersion());
  Serial.print("Firmware ");
  Serial.println(sim800l->getFirmware());

  // Print SIM card status
  Serial.print(F("SIM status "));
  Serial.println(sim800l->getSimStatus());

  // Print SIM card number
  Serial.print("SIM card number ");
  Serial.println(sim800l->getSimCardNumber());

  // Wait for the GSM signal
  uint8_t signal = sim800l->getSignal();
  while (signal <= 0) {
    delay(1000);
    signal = sim800l->getSignal();
  }

  if (signal > 5) {
    Serial.print(F("Signal OK (strenght: "));
  } else {
    Serial.print(F("Signal low (strenght: "));
  }
  Serial.print(signal);
  Serial.println(F(")"));
  delay(1000);

  // Wait for operator network registration (national or roaming network)
  NetworkRegistration network = sim800l->getRegistrationStatus();
  while (network != REGISTERED_HOME && network != REGISTERED_ROAMING) {
    delay(1000);
    network = sim800l->getRegistrationStatus();
  }
  Serial.println(F("Network registration OK"));
  sim800l->setupGPRS("internet");
  sim800l->connectGPRS();

  Serial.println("End of test protocol");
}

void getGPSData() {
  while (gpsSerial.available() > 0) {
    digitalWrite(internalLed, HIGH);
    Serial.println(gps.satellites.value());
    gps.encode(gpsSerial.read());
    if (gps.location.isUpdated()) {
      Lat     = gps.location.lat();    //HAVEN'T TRY THE "6-s and 2-s" PARAMETER, TRY THIS ONE IF SOMETHING WIERD HAPPENS
      Lon     = gps.location.lng();
      Alt     = gps.altitude.meters();
    }
    digitalWrite(internalLed, LOW);
  }
}


int detectRC522Module() {
  if (mfrc522.PCD_PerformSelfTest()) {
    return 0;

  }
  else {
    return 1;
  }

}

//void checkGpsSerial() {
//  if (!gpsSerial) { // If the object did not initialize, then its configuration is invalid
//    Serial.println("Invalid EspSoftwareSerial pin configuration, check config");
//    while (1) { // Don't continue with invalid configuration
//      delay (1000);
//    }
//  }
//}

void solenoidControl(String UID) {
  if (compareUID == UID) {
    digitalWrite(solenoidPin, HIGH);
    Serial.println("UNLOCKING SOLENOID");
    delay(5000);
    digitalWrite(solenoidPin, LOW);
    Serial.println("LOCKING SOLENOID");
  }
  else {
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



void startupTone() {
  digitalWrite(buzzerPin, HIGH);
  delay(150);
  digitalWrite(buzzerPin, LOW);
  delay(150);
  digitalWrite(buzzerPin, HIGH);
  delay(150);
  digitalWrite(buzzerPin, LOW);
  delay(150);
  digitalWrite(buzzerPin, HIGH);
  delay(150);
  digitalWrite(buzzerPin, LOW);
  delay(150);
  digitalWrite(buzzerPin, HIGH);
  delay(150);
  digitalWrite(buzzerPin, LOW);
  delay(150);
}
