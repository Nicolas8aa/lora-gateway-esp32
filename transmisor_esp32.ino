#include <WiFi.h>
#include "ESPAsyncWebServer.h"

#include <LittleFS.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

// LoRa pins
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

#define BAND 433E6  // Set according to your region


// NTP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables
String timestamp;

int rssi;
String loRaMessage;
String humidity;
String incoming;

// Web server on port 80
AsyncWebServer server(80);

// Get time from NTP
void getTimeStamp() {
  timeClient.update();  // This is non-blocking
  timestamp = timeClient.getFormattedTime();
}

void sendDataToApi (String incoming) {
  humidity = incoming.substring(8, incoming.indexOf('|', 8));
  rssi = LoRa.packetRssi();

  Serial.print(humidity);
  Serial.print(" with RSSI ");
  Serial.println(rssi);

  timestamp = timeClient.getFormattedTime();

  StaticJsonDocument<200> jsonDoc;
  jsonDoc["porcentaje_humedad"] = humidity.toFloat();  // convert to float if needed
  jsonDoc["clave_sensor"] = SENSOR_KEY;
  //jsonDoc["timestamp"] = timestamp;

  String requestBody;
  serializeJson(jsonDoc, requestBody);

  // Send HTTP POST request
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(SERVER_URL); // replace with your API URL
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
      Serial.print("POST Response code: ");
      Serial.println(httpResponseCode);
      String response = http.getString();
      Serial.println("Response: " + response);
    } else {
      Serial.print("POST failed: ");
      Serial.println(http.errorToString(httpResponseCode));
    }

    http.end();

}
}

// Get humidity from LoRa packet
void getLoRaData() {

  Serial.print("Lora packet received: ");

  while (LoRa.available()) {
    String incoming = LoRa.readString();  // safer read
    incoming.trim();


    if (incoming.startsWith(LORA_PREFIX)) {
      sendDataToApi(incoming);
    } else {
      Serial.println("Invalid packet received.");
    }
  }
}


// HTTP placeholder replacements
String processor(const String &var) {
  if (var == "HUMIDITY") return humidity;
  if (var == "TIMESTAMP") return timestamp;
  if (var == "RRSI") return String(rssi);
  return String();
}

void connectWiFi() {
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void startLoRa() {
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  int counter = 0;

  while (!LoRa.begin(BAND) && counter < 10) {
    Serial.print(".");
    counter++;
    delay(500);
  }

  if (counter == 10) {
    Serial.println("Starting LoRa failed!");
    while (true)
      ;
  }


  Serial.println("LoRa Initialization OK!");
  delay(2000);
}

void setup() {
  Serial.begin(115200);
  startLoRa();
  connectWiFi();

  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    return;
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", String(), false, processor);
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", humidity.c_str());
  });
  server.on("/timestamp", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", timestamp.c_str());
  });
  server.on("/rssi", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(rssi).c_str());
  });

  server.on("/winter", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/winter.jpg", "image/jpg");
  });

  server.begin();

  timeClient.begin();
  timeClient.setTimeOffset(-18000);  // Adjust for your time zone if needed
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    getLoRaData();
    getTimeStamp();
  }
  //yield();  // allow Wi-Fi and background tasks to run
}