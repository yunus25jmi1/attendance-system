#include <ESP8266WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <RFID.h>
#include <FirebaseESP8266.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

// WiFi credentials (to be replaced with a secure method)
const char* ssid = "Your_WiFi_SSID";
const char* password = "Your_WiFi_Password";

// Firebase configuration (to be replaced with a secure method)
#define FIREBASE_HOST "mqtt-b9578.firebaseio.com"
#define FIREBASE_AUTH "zeQcXZPtKdHZhKZe5DrX1oTgogiFZ1DW7sB2KWCj"

// NTP time configuration
const long utcOffsetInSeconds = 19800; // UTC+5:30

// Pin assignments
const int redLedPin = D4;
const int greenLedPin = D3;
const int rfidRstPin = D0;
const int rfidSdaPin = D8;

// RFID reader
RFID rfid(rfidSdaPin, rfidRstPin);

// LCD display
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Firebase data object
FirebaseData firebaseData;

// NTP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Device ID (to be replaced with a dynamic method)
String deviceID = "device11";

// Function prototypes
void connectWiFi();
void initializeComponents();
void checkAccess(String uid);
void updateAttendance(String uid, int status);
void displayMessage(const String& message);

void setup() {
  Serial.begin(115200);
  initializeComponents();
  connectWiFi();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
}

void loop() {
  timeClient.update();

  if (rfid.findCard(PICC_REQIDL, str) == MI_OK) {
    String uid = "";
    if (rfid.anticoll(str) == MI_OK) {
      for (int i = 0; i < 4; i++) {
        uid += String(str[i], HEX);
      }
      checkAccess(uid);
    }
    rfid.selectTag(str);
  }

  rfid.halt();
  displayMessage("Scan your RFID");
  lcd.clear();
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected to WiFi");
}

void initializeComponents() {
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  lcd.init();
  lcd.backlight();
  SPI.begin();
  rfid.init();
  timeClient.begin();
  timeClient.setTimeOffset(utcOffsetInSeconds);
}

void checkAccess(String uid) {
  displayMessage("Scanning RFID");

  if (Firebase.getInt(firebaseData, "/users/" + uid)) {
    int status = firebaseData.intData();
    if (status == 0) {
      updateAttendance(uid, 1);
      displayMessage("Checking In");
    } else if (status == 1) {
      updateAttendance(uid, 0);
      displayMessage("Checking Out");
    }
  } else {
    Serial.println("Failed to get user status: " + firebaseData.errorReason());
  }
}

void updateAttendance(String uid, int status) {
  StaticJsonDocument<200> json;
  json["time"] = timeClient.getFormattedDate();
  json["id"] = deviceID;
  json["uid"] = uid;
  json["status"] = status;

  if (Firebase.pushJSON(firebaseData, "/attendance", json)) {
    Serial.println("Attendance updated: " + firebaseData.dataPath() + firebaseData.pushName());
  } else {
    Serial.println("Failed to update attendance: " + firebaseData.errorReason());
  }

  if (Firebase.setInt(firebaseData, "/users/" + uid, status)) {
    Serial.println("User status updated: " + uid + " - " + String(status));
  } else {
    Serial.println("Failed to update user status: " + firebaseData.errorReason());
  }
}

void displayMessage(const String& message) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);
}
