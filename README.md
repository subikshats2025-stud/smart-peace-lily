# Smart Peace Lily – IoT-Based Plant Wellness Monitoring System

An IoT-based plant monitoring system designed to monitor the wellness of a Peace Lily using an ESP32 and multiple environmental sensors.

The system continuously monitors temperature, humidity, light intensity, and soil moisture. It evaluates the sensor readings against predefined Peace Lily thresholds and provides plant health recommendations. Sensor data and alerts are also transmitted using MQTT.

---

## Project Overview

The Smart Peace Lily system uses an ESP32 as the main controller.

The system monitors:

- Temperature
- Air Humidity
- Light Intensity
- Soil Moisture

The collected data is processed by the ESP32 and classified into suitable plant conditions such as GOOD, TOO LOW, TOO HIGH, DRY, TOO WET, TOO DARK, or TOO BRIGHT.

The system also generates an overall plant status:

- `HEALTHY`
- `NEEDS_ATTENTION`

MQTT is used to publish sensor data, plant status, and custom alerts.

---

## Features

- Real-time temperature monitoring
- Real-time humidity monitoring
- Light intensity monitoring
- Soil moisture monitoring
- Peace Lily-specific threshold evaluation
- Automatic plant health status
- Watering recommendation when soil is dry
- Light condition recommendations
- Custom alert when soil is dry despite adequate humidity
- MQTT-based IoT communication
- Serial Monitor output
- Wokwi-based ESP32 simulation

---

## Hardware Components

- ESP32
- DHT22 Temperature and Humidity Sensor
- LDR Light Sensor Module
- Soil Moisture Sensor
- Connecting wires

---

## Software and Tools

- Arduino / ESP32
- Wokwi Simulator
- MQTT
- HiveMQ Public MQTT Broker
- GitHub

### Libraries Used

- DHTesp
- WiFi
- PubSubClient

---

## Pin Connections

| Component | ESP32 Pin |
|-----------|-----------|
| DHT22 Data | GPIO 15 |
| LDR Analog Output | GPIO 34 |
| Soil Moisture Analog Output | GPIO 35 |

---

## Peace Lily Thresholds

The system uses the following threshold values:

| Parameter | Range / Condition |
|-----------|-------------------|
| Temperature | 20°C – 28°C |
| Humidity | 40% – 70% |
| Soil Moisture | 30% – 70% |
| Light | Based on calibrated LDR ADC values |

The soil moisture sensor provides an ADC value from 0 to 4095, which is converted into a percentage.

---

## Working Principle

1. The ESP32 initializes the connected sensors.
2. The ESP32 connects to the Wi-Fi network.
3. The ESP32 connects to the MQTT broker.
4. Temperature and humidity are read from the DHT22.
5. Light intensity is read using the LDR.
6. Soil moisture is read using the soil moisture sensor.
7. The sensor values are compared with the predefined Peace Lily thresholds.
8. The system determines the condition of each parameter.
9. An overall plant status is generated.
10. Sensor data is published through MQTT.
11. Plant status is published through MQTT.
12. Custom alerts are published when specific conditions occur.

---

## Custom Alert

The system includes a custom plant-health condition.

If:

- Soil moisture is below the required level
- AND air humidity is adequate

the system generates:

`DRY_SOIL_ADEQUATE_HUMIDITY`

and recommends:

`WATER_PLANT`

This allows the system to provide a more meaningful plant-care recommendation rather than simply displaying raw sensor values.

---

## MQTT Communication

The system uses MQTT for IoT communication.

### MQTT Broker

```text
broker.hivemq.com
