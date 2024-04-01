#include <Arduino.h>
#include "MHZ19.h"                                        
#include <HardwareSerial.h>
#include <WiFi.h>                      
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <esp_wifi.h>
#include <vector>

#define BAUDRATE 9600                                      // Device to MH-Z19 Serial baudrate (should not be changed)
const char* ssid = "Wifi SSID";                                 // WiFi SSID
const char* password = "WIFI PASSWORD";                         // WiFi Password
const char* api_url = "https://backend.thinger.io/v3/users/...";                    // Thinger.io host
const String token = "Bearer ABCABCABC";

MHZ19 myMHZ19;                                             // Constructor for library
HardwareSerial mySerial(2);                   
WiFiClientSecure client;

std::vector<String> offlineData;

unsigned long getDataTimer = 0;

void setup()
{
    Serial.begin(9600);                                     // Device to serial monitor feedback

    mySerial.begin(BAUDRATE);                               
    myMHZ19.begin(mySerial);                                // *Serial(Stream) refence must be passed to library begin(). 
    myMHZ19.autoCalibration();                              // Turn auto calibration ON (OFF autoCalibration(false))

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      delay(1000);
    }
    Serial.println(WiFi.localIP());
    client.setInsecure();
}

void sendData(String data) {
  HTTPClient http;
  http.begin(client, api_url);
  http.addHeader("Content-Type", "application/json;charset=UTF-8");
  http.addHeader("Authorization", token);
  int httpCode = http.POST(data);
  if (httpCode != 200 && httpCode > 0 ) {
      String payload = http.getString();
      Serial.println(httpCode);
      Serial.println(payload);
  }
  else {
      Serial.println(http.errorToString(httpCode).c_str());
  }
  http.end();
}

void loop()
{
    if (millis() - getDataTimer >= 61000)
    {
        JsonDocument  doc;
        String        data;
        
        doc["co2"] = myMHZ19.getCO2();
        doc["temperature"] = (int)myMHZ19.getTemperature();
        doc["startup_time"] = millis() / 1000;
        serializeJson(doc, data);
        if (WiFi.status() == WL_CONNECTED)
        {
          // if (offlineData.size() > 0) {
          //   for (auto& d : offlineData) {
          //     sleep(0.1);
          //     sendData(d);
          //   }
          //   offlineData.clear();
          // }
          // Send the current data
          sendData(data);
        } else {
            // If WiFi is not connected, store the data for later
            // if (offlineData.size() > 800) {
            //   offlineData.erase(offlineData.begin());
            // }
            offlineData.push_back(data);
            WiFi.begin(ssid, password);
        }
    }
}