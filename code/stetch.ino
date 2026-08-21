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

int previousSoilPercent = -1;

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

  Serial.println("========================================");
  Serial.println("     PEACE LILY SMART MONITOR");
  Serial.println("========================================");

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

    TempAndHumidity data = dhtSensor.getTempAndHumidity();

    int lightValue = analogRead(LDR_PIN);
    int soilValue = analogRead(SOIL_PIN);

    int soilPercent = map(soilValue, 0, 4095, 0, 100);

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
      lightValue >= LIGHT_BRIGHT &&
      lightValue <= LIGHT_DARK;

    String temperatureStatus;
    String humidityStatus;
    String soilStatus;
    String lightStatus;

    if (temperatureOK) {
      temperatureStatus = "GOOD";
    } else if (data.temperature < TEMP_MIN) {
      temperatureStatus = "TOO LOW";
    } else {
      temperatureStatus = "TOO HIGH";
    }

    if (humidityOK) {
      humidityStatus = "GOOD";
    } else if (data.humidity < HUMIDITY_MIN) {
      humidityStatus = "TOO LOW";
    } else {
      humidityStatus = "TOO HIGH";
    }

    if (soilOK) {
      soilStatus = "GOOD";
    } else if (soilPercent < SOIL_MIN) {
      soilStatus = "DRY";
    } else {
      soilStatus = "TOO WET";
    }

    if (lightOK) {
      lightStatus = "GOOD";
    } else if (lightValue < LIGHT_BRIGHT) {
      lightStatus = "TOO BRIGHT";
    } else {
      lightStatus = "TOO DARK";
    }

    bool moistureImbalance =
      soilPercent < SOIL_MIN &&
      data.humidity > HUMIDITY_MAX;

    bool soilImproving = false;
    bool soilDeteriorating = false;

    if (previousSoilPercent >= 0) {
      if (previousSoilPercent < SOIL_MIN &&
          soilPercent >= SOIL_MIN) {
        soilImproving = true;
      }

      if (previousSoilPercent >= SOIL_MIN &&
          soilPercent < SOIL_MIN) {
        soilDeteriorating = true;
      }
    }

    String overallStatus = "HEALTHY";

    if (!temperatureOK ||
        !humidityOK ||
        !soilOK ||
        !lightOK ||
        moistureImbalance) {
      overallStatus = "NEEDS ATTENTION";
    }

    Serial.println();
    Serial.println("========== PEACE LILY STATUS ==========");

    Serial.print("Temperature   : ");
    Serial.print(data.temperature, 1);
    Serial.print(" °C   [");
    Serial.print(temperatureStatus);
    Serial.println("]");

    Serial.print("Humidity      : ");
    Serial.print(data.humidity, 1);
    Serial.print(" %    [");
    Serial.print(humidityStatus);
    Serial.println("]");

    Serial.print("Soil Moisture : ");
    Serial.print(soilPercent);
    Serial.print(" %       [");
    Serial.print(soilStatus);
    Serial.println("]");

    Serial.print("Light         : ");
    Serial.print(lightValue);
    Serial.print("      [");
    Serial.print(lightStatus);
    Serial.println("]");

    Serial.println("----------------------------------------");

    Serial.print("PLANT STATUS : ");
    Serial.println(overallStatus);

    if (soilPercent < SOIL_MIN) {
      Serial.println("RECOMMENDATION: Check soil moisture");
    } else if (soilPercent > SOIL_MAX) {
      Serial.println("RECOMMENDATION: Avoid overwatering");
    } else if (!lightOK) {
      if (lightValue < LIGHT_BRIGHT) {
        Serial.println("RECOMMENDATION: Move plant to indirect light");
      } else {
        Serial.println("RECOMMENDATION: Move plant away from strong light");
      }
    }

    Serial.println("----------------------------------------");

    bool customAlert = false;

    if (moistureImbalance) {
      customAlert = true;

      Serial.println("CUSTOM CONDITION DETECTED");
      Serial.println("Moisture imbalance detected");
      Serial.println("Dry soil despite humid air");
      Serial.println("ACTION: Check soil/root condition");
      Serial.println("----------------------------------------");
    }

    if (soilImproving) {
      Serial.println("TREND ANALYSIS");
      Serial.print("Soil Moisture: ");
      Serial.print(previousSoilPercent);
      Serial.print("% -> ");
      Serial.print(soilPercent);
      Serial.println("%");
      Serial.println("PLANT CONDITION: IMPROVING");
      Serial.println("----------------------------------------");
    }

    if (soilDeteriorating) {
      Serial.println("TREND ANALYSIS");
      Serial.print("Soil Moisture: ");
      Serial.print(previousSoilPercent);
      Serial.print("% -> ");
      Serial.print(soilPercent);
      Serial.println("%");
      Serial.println("PLANT CONDITION: DETERIORATING");
      Serial.println("ACTION: Check watering conditions");
      Serial.println("----------------------------------------");
    }

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

    Serial.println("MQTT");
    Serial.print("Data published");

    if (customAlert) {
      String alertMessage =
        "{\"alert\":\"MOISTURE_IMBALANCE\","
        "\"action\":\"CHECK_SOIL_ROOT_CONDITION\"}";

      mqttClient.publish(
        MQTT_ALERT_TOPIC,
        alertMessage.c_str()
      );

      Serial.println(" + alert");
      Serial.println();
      Serial.println("MQTT: Sensor data published!");
      Serial.println("MQTT: Custom alert published!");
    } else {
      Serial.println();
      Serial.println();
      Serial.println("MQTT: Sensor data published!");
    }

    Serial.println("========================================");

    previousSoilPercent = soilPercent;
  }
}
