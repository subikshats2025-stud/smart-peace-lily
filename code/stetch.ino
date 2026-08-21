#include "DHTesp.h"
#include <WiFi.h>
#include <PubSubClient.h>

#define DHT_PIN 15
#define LDR_PIN 34
#define SOIL_PIN 35

DHTesp dhtSensor;

#define TEMP_MIN 20
#define TEMP_MAX 28

#define HUMIDITY_MIN 40
#define HUMIDITY_MAX 70

#define SOIL_MIN 30
#define SOIL_MAX 70

#define LIGHT_DARK 2404
#define LIGHT_BRIGHT 1234

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

const char* MQTT_SERVER = "broker.hivemq.com";
const int MQTT_PORT = 1883;

const char* MQTT_SENSOR_TOPIC = "peace_lily/sensors";
const char* MQTT_STATUS_TOPIC = "peace_lily/status";
const char* MQTT_ALERT_TOPIC = "peace_lily/alerts";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastPublish = 0;
const unsigned long PUBLISH_INTERVAL = 5000;

void connectWiFi() {

  Serial.println();
  Serial.print("Connecting to WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

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

  while (!mqttClient.connected()) {

    Serial.print("Connecting to MQTT...");

    String clientID = "PeaceLilyESP32-";
    clientID += String(random(0xffff), HEX);

    if (mqttClient.connect(clientID.c_str())) {

      Serial.println("connected!");

    } else {

      Serial.print("failed, state = ");
      Serial.println(mqttClient.state());

      delay(3000);
    }
  }
}

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("   PEACE LILY SMART MONITOR");
  Serial.println("================================");

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  connectWiFi();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  connectMQTT();
}

void loop() {

  if (!mqttClient.connected()) {
    connectMQTT();
  }

  mqttClient.loop();

  if (millis() - lastPublish >= PUBLISH_INTERVAL) {

    lastPublish = millis();

    TempAndHumidity data =
      dhtSensor.getTempAndHumidity();

    int lightValue = analogRead(LDR_PIN);

    int soilValue = analogRead(SOIL_PIN);

    int soilPercent =
      map(soilValue, 0, 4095, 0, 100);

    bool temperatureOK =
      data.temperature >= TEMP_MIN &&
      data.temperature <= TEMP_MAX;

    bool humidityOK =
      data.humidity >= HUMIDITY_MIN &&
      data.humidity <= HUMIDITY_MAX;

    bool soilOK =
      soilPercent >= SOIL_MIN &&
      soilPercent <= SOIL_MAX;

    bool lightOK =
      lightValue <= LIGHT_DARK &&
      lightValue >= LIGHT_BRIGHT;

    String overallStatus = "HEALTHY";

    Serial.println();
    Serial.println("===== PEACE LILY =====");

    Serial.print("Temperature: ");
    Serial.print(data.temperature);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(data.humidity);
    Serial.println(" %");

    Serial.print("Light ADC: ");
    Serial.println(lightValue);

    Serial.print("Soil Moisture: ");
    Serial.print(soilPercent);
    Serial.println(" %");

    if (!temperatureOK) {

      overallStatus = "NEEDS_ATTENTION";

      if (data.temperature < TEMP_MIN) {
        Serial.println("Temperature: TOO LOW");
      } else {
        Serial.println("Temperature: TOO HIGH");
      }

    } else {

      Serial.println("Temperature: GOOD");
    }

    if (!humidityOK) {

      overallStatus = "NEEDS_ATTENTION";

      if (data.humidity < HUMIDITY_MIN) {
        Serial.println("Humidity: TOO LOW");
      } else {
        Serial.println("Humidity: TOO HIGH");
      }

    } else {

      Serial.println("Humidity: GOOD");
    }

    if (!soilOK) {

      overallStatus = "NEEDS_ATTENTION";

      if (soilPercent < SOIL_MIN) {

        Serial.println("Soil: DRY");
        Serial.println("Action: Water the Peace Lily");

      } else {

        Serial.println("Soil: TOO WET");
        Serial.println("Action: Avoid overwatering");
      }

    } else {

      Serial.println("Soil: GOOD");
    }

    if (!lightOK) {

      overallStatus = "NEEDS_ATTENTION";

      if (lightValue > LIGHT_DARK) {

        Serial.println("Light: TOO DARK");
        Serial.println(
          "Action: Move plant to brighter indirect light"
        );

      } else {

        Serial.println("Light: TOO BRIGHT");
        Serial.println(
          "Action: Move plant away from strong light"
        );
      }

    } else {

      Serial.println("Light: GOOD");
    }

    bool customAlert = false;

    if (soilPercent < SOIL_MIN &&
        data.humidity >= HUMIDITY_MIN) {

      customAlert = true;
      overallStatus = "NEEDS_ATTENTION";

      Serial.println();
      Serial.println("CUSTOM ALERT!");

      Serial.println(
        "Soil is dry despite adequate air humidity."
      );

      Serial.println(
        "Action: Water the Peace Lily."
      );
    }

    Serial.println();

    Serial.print("PLANT STATUS: ");
    Serial.println(overallStatus);

    Serial.println("======================");

    String sensorMessage = "{";

    sensorMessage += "\"temperature\":";
    sensorMessage += String(data.temperature, 1);

    sensorMessage += ",\"humidity\":";
    sensorMessage += String(data.humidity, 1);

    sensorMessage += ",\"light_adc\":";
    sensorMessage += String(lightValue);

    sensorMessage += ",\"soil_moisture\":";
    sensorMessage += String(soilPercent);

    sensorMessage += "}";

    mqttClient.publish(
      MQTT_SENSOR_TOPIC,
      sensorMessage.c_str()
    );

    String statusMessage =
      "{\"status\":\"" + overallStatus + "\"}";

    mqttClient.publish(
      MQTT_STATUS_TOPIC,
      statusMessage.c_str()
    );

    if (customAlert) {

      String alertMessage =
        "{\"alert\":\"DRY_SOIL_ADEQUATE_HUMIDITY\","
        "\"action\":\"WATER_PLANT\"}";

      mqttClient.publish(
        MQTT_ALERT_TOPIC,
        alertMessage.c_str()
      );

      Serial.println(
        "MQTT: Custom alert published!"
      );
    }

    Serial.println(
      "MQTT: Sensor data published!"
    );
  }
}
