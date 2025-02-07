/*
    HTTP REST API POST/GET over TLS (HTTPS)
    Created By: Zachary Hunter / zachary.hunter@gmail.com
    Created On: April 2, 2024
    Verified Working on NodeMCU & NodeMCU MMINI on ESP8266
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Arduino_JSON.h>
#include "certs.h"

// Setup Sensor PIN & Variables
int SensorPIN = D3;
int led = LED_BUILTIN;
int currentState = LOW;
int val = 0;

// Timer Setup (1000ms = 1second), setup for 1HR
unsigned long timerDelay = (1000 * 60 * 60);
unsigned long lastTime = millis() - timerDelay;

// ESP8266 WIFI Setup
const char* ssid = "AutumnHillsGuest";
const char* password = "Hunter2023";

// REST Web Server Settings
const char* serverName = "https://www.zachhunter.net/wp-json/wp-arduino-api/v1/arduino";
String authorizationKey = "QT1kIG0Dt9u090ODH6bHXvGU";
X509List cert(cert_ISRG_Root_X1);

void setup() {
  Serial.begin(115200);

  ConnectWifi();

  Serial.print("NodeMCU POST Sensor Value Every ");
  Serial.print(timerDelay / 1000);
  Serial.println(" seconds.");

  pinMode(SensorPIN, INPUT);
  //currentState = digitalRead(SensorPIN);
  //currentState = analogRead(SensorPIN);
}

//declare reset function at address 0 - MUST BE ABOVE LOOP
void (*resetFunc)(void) = 0;

void loop(){
  val = digitalRead(SensorPIN);   // read sensor value
  Serial.println(val);

  if (val == HIGH) {           // check if the sensor is HIGH
    digitalWrite(led, HIGH);   // turn LED ON
    delay(100);                // delay 100 milliseconds 
    
    if (currentState == LOW) {
      Serial.println("Motion detected!"); 
      currentState = HIGH;       // update variable state to HIGH
    }
  } 
  else {
      digitalWrite(led, LOW); // turn LED OFF
      delay(200);             // delay 200 milliseconds 
      
      if (currentState == HIGH){
        Serial.println("Motion stopped!");
        currentState = LOW;       // update variable state to LOW
    }
  }
}

// void loop() {
//   // Every 15 Seconds Read Sensor Value
//   if (millis() > lastTime + 15000) {
//     currentState = analogRead(SensorPIN);
//   }

//   // Every X Seconds or Change of Sensor Value
//   if (millis() > lastTime + timerDelay || abs(lastState - currentState) > 100) {
//     if (WiFi.status() == WL_CONNECTED) {
//       lastState = currentState;
//       lastTime = millis();

//       RecordMoisture();
//       httpGETRequest(serverName);
//     } else {
//       lastState = currentState;
//       lastTime = millis();

//       WiFi.printDiag(Serial);
//       RecordMoisture();
//     }
//   }
// }

void PostSensorData(JSONVar myObject) {
  // ========================================================
  // POST Sensor Values to the DB via REST API POST Method
  // ========================================================
  String jsonString = JSON.stringify(myObject);
  String httpResponse = httpPOSTRequest(serverName, jsonString);
  Serial.println("HTTP Response: " + httpResponse);
  Serial.println("==============================================================================");
}

// Record Moisture Sensor
JSONVar RecordMotion() {
  String textSensorStatus;

  long state = digitalRead(SensorPIN);
  if (state == HIGH) {
    digitalWrite(BUILTIN_LED, HIGH);
    textSensorStatus = "Motion detected!";
    delay(1000);
  } else {
    digitalWrite(BUILTIN_LED, LOW);
    textSensorStatus = "Motion absent!";
    delay(1000);
  }

  Serial.println(textSensorStatus);

  JSONVar myObject;

  myObject["sendorValue"] = WiFi.localIP().toString();
  myObject["sensorType"] = "MotionSensor";
  myObject["sensorValue"] = textSensorStatus;

  return myObject;
}

// Record Moisture Sensor
JSONVar RecordMoisture() {
  String textSensorStatus;

  if (currentState >= 900) {
    textSensorStatus = "Dry " + String(currentState);
  } else if (currentState >= 800) {
    textSensorStatus = "Damp " + String(currentState);
  } else if (currentState >= 600) {
    textSensorStatus = "Moist " + String(currentState);
  } else if (currentState >= 0) {
    textSensorStatus = "Soaked " + String(currentState);
  }

  Serial.println(textSensorStatus);

  JSONVar myObject;

  myObject["sendorValue"] = WiFi.localIP().toString();
  myObject["sensorType"] = "MoistureSensor";
  myObject["sensorValue"] = textSensorStatus;

  return myObject;
}

// Connect to WIFI - Leave Connection Open
void ConnectWifi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting ");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  // Set time via NTP, as required for x.509 validation
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

// POST JSON Data to REST API
String httpPOSTRequest(const char* serverName, String httpRequestData) {
  WiFiClientSecure client;
  HTTPClient http;

  client.setTrustAnchors(&cert);

  // Enable if your having certificate issues
  //client.setInsecure();

  Serial.println("Secure POST Request to: " + String(serverName));
  Serial.println("Payload: " + httpRequestData);

  http.begin(client, serverName);
  http.addHeader("Authorization", authorizationKey);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(httpRequestData);

  String payload = "{}";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  Serial.println();

  http.end();

  return payload;
}

// GET Request to REST API
String httpGETRequest(const char* serverName) {
  WiFiClientSecure client;
  HTTPClient http;

  client.setTrustAnchors(&cert);

  // Enable if your having certificate issues
  //client.setInsecure();

  Serial.println("Secure GET Request to: " + String(serverName));

  http.begin(client, serverName);
  http.addHeader("Authorization", authorizationKey);

  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  Serial.println();

  http.end();

  return payload;
}
