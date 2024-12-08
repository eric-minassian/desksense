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

  setupWiFi();
}

void loop() {
  auto& display = DisplayManager::getInstance();
  display.clearScreen();

  int yPos = 5;

  if (tempSensor.measure()) {
    tempSensor.printMeasurements();
    tempSensor.displayMeasurements(yPos);
  }

  if (airSensor.measure()) {
    airSensor.printMeasurements();
    airSensor.displayMeasurements(yPos);
  }

  if (micSensor.measure()) {
    micSensor.printMeasurements();
    micSensor.displayMeasurements(yPos);
  }

  if (lightSensor.measure()) {
    lightSensor.printMeasurements();
    lightSensor.displayMeasurements(yPos);
  }

  display.pushToDisplay();
  delay(1000);

  static unsigned long lastSendTime = 0;
  const unsigned long SEND_INTERVAL = 15000;  // Send every 15 seconds

  if (millis() - lastSendTime >= SEND_INTERVAL) {
    sendSensorData();
    lastSendTime = millis();
  }
}