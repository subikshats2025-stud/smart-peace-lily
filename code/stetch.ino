#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#define DHTPIN 15
#define DHTTYPE DHT22
#define LDR_PIN 34
#define SOIL_PIN 35

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

float previousHumidity = -1;
float previousSoilMoisture = -1;

void connectWiFi() {
  Serial.print("Connecting to WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");

    if (client.connect("PeaceLilyESP32")) {
      Serial.println("connected!");
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  dht.begin();

  connectWiFi();

  client.setServer(mqtt_server, 1883);
  connectMQTT();

  Serial.println();
  Serial.println("========================================");
  Serial.println("          PEACE LILY MONITOR");
  Serial.println("========================================");
}

void loop() {
  if (!client.connected()) {
    connectMQTT();
  }

  client.loop();

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  int lightADC = analogRead(LDR_PIN);
  int soilADC = analogRead(SOIL_PIN);

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Sensor reading error!");
    delay(2000);
    return;
  }

  float soilMoisture = (soilADC * 100.0) / 4095.0;

  String tempStatus;
  String humidityStatus;
  String soilStatus;
  String lightStatus;

  if (temperature < 20) {
    tempStatus = "TOO LOW";
  } else if (temperature > 28) {
    tempStatus = "TOO HIGH";
  } else {
    tempStatus = "GOOD";
  }

  if (humidity < 40) {
    humidityStatus = "TOO LOW";
  } else if (humidity > 70) {
    humidityStatus = "TOO HIGH";
  } else {
    humidityStatus = "GOOD";
  }

  if (soilMoisture < 30) {
    soilStatus = "DRY";
  } else if (soilMoisture > 70) {
    soilStatus = "TOO WET";
  } else {
    soilStatus = "GOOD";
  }

  if (lightADC < 1234) {
    lightStatus = "TOO BRIGHT";
  } else if (lightADC > 2404) {
    lightStatus = "TOO DARK";
  } else {
    lightStatus = "GOOD";
  }

  bool needsAttention =
    tempStatus != "GOOD" ||
    humidityStatus != "GOOD" ||
    soilStatus != "GOOD" ||
    lightStatus != "GOOD";

  String recommendation = "Conditions are suitable";

  if (soilStatus == "DRY") {
    recommendation = "Water the plant";
  } else if (soilStatus == "TOO WET") {
    recommendation = "Avoid overwatering";
  } else if (lightStatus == "TOO BRIGHT") {
    recommendation = "Move plant away from strong light";
  } else if (lightStatus == "TOO DARK") {
    recommendation = "Move plant into brighter light";
  } else if (tempStatus == "TOO LOW") {
    recommendation = "Move plant to a warmer location";
  } else if (tempStatus == "TOO HIGH") {
    recommendation = "Move plant to a cooler location";
  } else if (humidityStatus == "TOO LOW") {
    recommendation = "Increase surrounding humidity";
  } else if (humidityStatus == "TOO HIGH") {
    recommendation = "Improve air circulation";
  }

  bool customAlert = false;

  Serial.println();
  Serial.println("========================================");
  Serial.println("          PEACE LILY STATUS");
  Serial.println("========================================");

  Serial.print("Temperature   : ");
  Serial.print(temperature, 1);
  Serial.print(" °C   [");
  Serial.print(tempStatus);
  Serial.println("]");

  Serial.print("Humidity      : ");
  Serial.print(humidity, 1);
  Serial.print(" %    [");
  Serial.print(humidityStatus);
  Serial.println("]");

  Serial.print("Soil Moisture : ");
  Serial.print(soilMoisture, 0);
  Serial.print(" %    [");
  Serial.print(soilStatus);
  Serial.println("]");

  Serial.print("Light         : ");
  Serial.print(lightADC);
  Serial.print("      [");
  Serial.print(lightStatus);
  Serial.println("]");

  Serial.println("----------------------------------------");

  Serial.print("PLANT STATUS  : ");
  Serial.println(needsAttention ? "NEEDS ATTENTION" : "HEALTHY");

  Serial.print("RECOMMENDATION: ");
  Serial.println(recommendation);

  Serial.println("----------------------------------------");

  if (soilMoisture < 30 && humidity >= 40 && humidity <= 70) {
    customAlert = true;

    Serial.println("CUSTOM CONDITION: WEATHER-ADAPTING MOISTURE");
    Serial.println("Soil is dry despite adequate air humidity.");
    Serial.println("ACTION: Water the plant and monitor moisture response.");
  }

  if (soilMoisture < 30 && humidity > 70) {
    customAlert = true;

    Serial.println("CUSTOM CONDITION: POSSIBLE ROOT ISSUE");
    Serial.println("Soil moisture is low while humidity is high.");
    Serial.println("ACTION: Check the root condition before watering further.");
  }

  if (previousHumidity >= 0 && humidity > previousHumidity + 2) {
    Serial.println("CUSTOM CONDITION: HUMIDITY IMPROVING");
    Serial.print("Humidity increased from ");
    Serial.print(previousHumidity, 1);
    Serial.print(" % to ");
    Serial.print(humidity, 1);
    Serial.println(" %.");
  }

  if (previousSoilMoisture >= 0 &&
      soilMoisture > previousSoilMoisture + 5) {
    Serial.println("CUSTOM CONDITION: SOIL MOISTURE IMPROVING");
    Serial.print("Soil moisture increased from ");
    Serial.print(previousSoilMoisture, 0);
    Serial.print(" % to ");
    Serial.print(soilMoisture, 0);
    Serial.println(" %.");
  }

  Serial.println("----------------------------------------");
  Serial.println("MQTT");

  String sensorData =
    "{\"temperature\":" + String(temperature, 1) +
    ",\"humidity\":" + String(humidity, 1) +
    ",\"light_adc\":" + String(lightADC) +
    ",\"soil_moisture\":" + String(soilMoisture, 0) + "}";

  client.publish("peace_lily/sensors", sensorData.c_str());
  Serial.println("Sensor data published!");

  String statusData =
    "{\"status\":\"" +
    String(needsAttention ? "NEEDS_ATTENTION" : "HEALTHY") +
    "\"}";

  client.publish("peace_lily/status", statusData.c_str());
  Serial.println("Plant status published!");

  if (customAlert) {
    String alertData =
      "{\"alert\":\"CUSTOM_PLANT_CONDITION\",\"action\":\"CHECK_PLANT\"}";

    client.publish("peace_lily/alerts", alertData.c_str());
    Serial.println("Custom alert published!");
  }

  Serial.println("========================================");

  previousHumidity = humidity;
  previousSoilMoisture = soilMoisture;

  delay(5000);
}
