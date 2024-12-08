#include <HTTPClient.h>
#include <WiFi.h>

#include "DHT20Sensor.h"
#include "DisplayManager.h"
#include "MAX9814.h"
#include "PhotoresistorSensor.h"
#include "SGP30Sensor.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

SGP30Sensor airSensor;
DHT20Sensor tempSensor;
MAX9814Sensor micSensor(36);
PhotoresistorSensor lightSensor(37);

const char* url = "https://desksense.vercel.app/api/sensor";

const int GREEN_LED = 15;
const int YELLOW_LED = 13;
const int RED_LED = 12;

const float TEMP_YELLOW = 25.0;   // °C
const float TEMP_RED = 30.0;      // °C
const float HUMID_YELLOW = 60.0;  // %
const float HUMID_RED = 70.0;     // %
const float LIGHT_YELLOW = 70.0;  // %
const float LIGHT_RED = 85.0;     // %
const float NOISE_YELLOW = 60.0;  // dB
const float NOISE_RED = 70.0;     // dB
const int ECO2_YELLOW = 1000;     // ppm
const int ECO2_RED = 2000;        // ppm
const int TVOC_YELLOW = 220;      // ppb
const int TVOC_RED = 660;         // ppb

enum AlertLevel { NORMAL = 0, WARNING = 1, ALERT = 2 };
AlertLevel currentAlertLevel = NORMAL;

char ssid[50];
char password[50];
char authHeader[60];

void nvs_access() {
  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  // Open
  Serial.printf("\n");
  Serial.printf("Opening Non-Volatile Storage (NVS) handle... ");
  nvs_handle_t my_handle;
  err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
  } else {
    Serial.printf("Done\n");
    Serial.printf("Retrieving SSID/PASSWD/AUTHHEADER\n");
    size_t ssid_len;
    size_t password_len;
    size_t authHeader_len;

    size_t required_size;
    err = nvs_get_str(my_handle, "authHeader", NULL, &required_size);
    if (err != ESP_OK) {
      Serial.printf("Error getting authHeader size: %s\n",
                    esp_err_to_name(err));
      return;
    }

    err = nvs_get_str(my_handle, "ssid", ssid, &ssid_len);
    err |= nvs_get_str(my_handle, "password", password, &password_len);
    err |= nvs_get_str(my_handle, "authHeader", authHeader, &authHeader_len);
    switch (err) {
      case ESP_OK:
        Serial.printf("Done\n");
        Serial.printf("SSID: %s\n", ssid);
        Serial.printf("PASSWORD: %s\n", password);
        Serial.printf("AUTHHEADER: %s\n", authHeader);
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        Serial.printf("The value is not initialized yet!\n");
        break;
      default:
        Serial.printf("Error (%s) reading!\n", esp_err_to_name(err));
    }
  }

  // Close
  nvs_close(my_handle);
}

void setupWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());
}

void sendSensorData() {
  if (WiFi.status() != WL_CONNECTED) return;

  char jsonBuffer[512];
  snprintf(jsonBuffer, sizeof(jsonBuffer),
           "{\"temperature\":%.2f,"
           "\"humidity\":%.2f,"
           "\"light\":%.2f,"
           "\"decibel\":%.2f,"
           "\"eCO2\":%d,"
           "\"TVOC\":%d,"
           "\"rawH2\":%d,"
           "\"rawEthanol\":%d}",
           tempSensor.getTemperatureCelsius(), tempSensor.getHumidity(),
           lightSensor.getLightPercentage(), micSensor.getDecibels(),
           airSensor.geteCO2(), airSensor.getTVOC(), airSensor.getRawH2(),
           airSensor.getRawEthanol());

  HTTPClient http;
  http.begin(url);

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", authHeader);

  int httpResponseCode = http.POST(jsonBuffer);

  if (httpResponseCode > 0) {
    Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    String payload = http.getString();
    Serial.println("Response payload: " + payload);
  } else {
    Serial.printf("Error code: %d\n", httpResponseCode);
  }

  http.end();
}

void updateLEDsAndDisplay(const char* sensor, float value, float yellowThresh,
                          float redThresh, int& yPos) {
  auto& display = DisplayManager::getInstance();
  char warningMsg[50];
  AlertLevel sensorLevel = NORMAL;

  if (value >= redThresh) {
    sensorLevel = ALERT;
    snprintf(warningMsg, sizeof(warningMsg), "%s ALERT!", sensor);
    display.drawText(warningMsg, 5, yPos, 2);
    yPos += 20;
  } else if (value >= yellowThresh) {
    sensorLevel = WARNING;
    snprintf(warningMsg, sizeof(warningMsg), "%s Warning", sensor);
    display.drawText(warningMsg, 5, yPos, 1);
    yPos += 10;
  }

  // Update the global alert level if this sensor's level is higher
  if (sensorLevel > currentAlertLevel) {
    currentAlertLevel = sensorLevel;
  }
}

void updateLEDs() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);

  switch (currentAlertLevel) {
    case ALERT:
      digitalWrite(RED_LED, HIGH);
      break;
    case WARNING:
      digitalWrite(YELLOW_LED, HIGH);
      break;
    case NORMAL:
      digitalWrite(GREEN_LED, HIGH);
      break;
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    delay(10);
  }

  delay(1000);

  nvs_access();

  auto& display = DisplayManager::getInstance();
  display.begin();

  Serial.println("Attempting to find SGP30 sensor...");
  if (!airSensor.begin()) {
    while (1) {
      Serial.println("Could not find SGP30 sensor. Please check wiring.");
      delay(5000);
    }
  }
  Serial.println("Found SGP30 sensor");

  Serial.println("Attempting to find DHT20 sensor...");
  if (!tempSensor.begin()) {
    while (1) {
      Serial.println("Could not find DHT20 sensor. Please check wiring.");
      delay(5000);
    }
  }
  Serial.println("Found DHT20 sensor");

  Serial.println("Attempting to find MAX9814 sensor...");
  if (!micSensor.begin()) {
    while (1) {
      Serial.println("Could not find MAX9814 sensor. Please check wiring.");
      delay(5000);
    }
  }
  Serial.println("Found MAX9814 sensor");
  micSensor.setDbRange(35.0, 80.0);

  Serial.println("Attempting to find Photoresistor sensor...");
  if (!lightSensor.begin()) {
    while (1) {
      Serial.println(
          "Could not find Photoresistor sensor. Please check wiring.");
      delay(5000);
    }
  }
  Serial.println("Found Photoresistor sensor");
  lightSensor.setThresholds(10, 40);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(GREEN_LED, HIGH);  // Start with green LED on

  setupWiFi();
}

void loop() {
  auto& display = DisplayManager::getInstance();
  display.clearScreen();

  // Reset the alert level at the start of each loop
  currentAlertLevel = NORMAL;

  int yPos = 5;

  if (tempSensor.measure()) {
    float temp = tempSensor.getTemperatureCelsius();
    float humidity = tempSensor.getHumidity();

    updateLEDsAndDisplay("TEMP", temp, TEMP_YELLOW, TEMP_RED, yPos);
    updateLEDsAndDisplay("HUMIDITY", humidity, HUMID_YELLOW, HUMID_RED, yPos);
    tempSensor.printMeasurements();
    tempSensor.displayMeasurements(yPos);
  }

  if (airSensor.measure()) {
    updateLEDsAndDisplay("ECO2", airSensor.geteCO2(), ECO2_YELLOW, ECO2_RED,
                         yPos);
    updateLEDsAndDisplay("TVOC", airSensor.getTVOC(), TVOC_YELLOW, TVOC_RED,
                         yPos);
    airSensor.printMeasurements();
    airSensor.displayMeasurements(yPos);
  }

  if (micSensor.measure()) {
    updateLEDsAndDisplay("NOISE", micSensor.getDecibels(), NOISE_YELLOW,
                         NOISE_RED, yPos);
    micSensor.printMeasurements();
    micSensor.displayMeasurements(yPos);
  }

  if (lightSensor.measure()) {
    updateLEDsAndDisplay("LIGHT", lightSensor.getLightPercentage(),
                         LIGHT_YELLOW, LIGHT_RED, yPos);
    lightSensor.printMeasurements();
    lightSensor.displayMeasurements(yPos);
  }

  updateLEDs();

  display.pushToDisplay();
  delay(1000);

  static unsigned long lastSendTime = 0;
  const unsigned long SEND_INTERVAL = 15000;  // Send every 15 seconds

  if (millis() - lastSendTime >= SEND_INTERVAL) {
    sendSensorData();
    lastSendTime = millis();
  }
}