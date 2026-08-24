#include "src/services/Mqtt.h"
#include "src/services/Network.h"
#include "src/hal/Battery.h"
#include "src/core/State.h"
#include "src/Config.h"

#include <WiFi.h>
#include <ArduinoJson.h>

namespace svc {

static WiFiClient s_mqttClient;
static bool       s_connected      = false;
static uint32_t   s_lastConnectTry = 0;
static uint32_t   s_lastPublishMs  = 0;
static uint32_t   s_lastPingMs     = 0;

static void sendMqttPacket(const uint8_t* data, size_t len) {
    if (s_mqttClient.connected()) {
        s_mqttClient.write(data, len);
    }
}

// Simple MQTT 3.1.1 packet builders
static void mqttConnect(const char* clientId, const char* user, const char* pass) {
    uint8_t varHeader[] = {
        0x00, 0x04, 'M', 'Q', 'T', 'T', // Protocol name
        0x04,                            // Level 4 (3.1.1)
        0x02,                            // Connect flags (Clean session)
        0x00, 0x3C                       // Keepalive 60s
    };

    uint8_t flags = 0x02;
    if (user && strlen(user) > 0) flags |= 0x80;
    if (pass && strlen(pass) > 0) flags |= 0x40;
    varHeader[7] = flags;

    // Calculate length
    uint16_t clientLen = strlen(clientId);
    uint16_t userLen   = (user && strlen(user) > 0) ? strlen(user) : 0;
    uint16_t passLen   = (pass && strlen(pass) > 0) ? strlen(pass) : 0;

    size_t payloadLen = 2 + clientLen;
    if (flags & 0x80) payloadLen += 2 + userLen;
    if (flags & 0x40) payloadLen += 2 + passLen;

    size_t remLen = sizeof(varHeader) + payloadLen;

    s_mqttClient.write((uint8_t)0x10); // CONNECT
    s_mqttClient.write((uint8_t)remLen);
    s_mqttClient.write(varHeader, sizeof(varHeader));

    // ClientId
    s_mqttClient.write((uint8_t)(clientLen >> 8));
    s_mqttClient.write((uint8_t)(clientLen & 0xFF));
    s_mqttClient.write((const uint8_t*)clientId, clientLen);

    // User
    if (flags & 0x80) {
        s_mqttClient.write((uint8_t)(userLen >> 8));
        s_mqttClient.write((uint8_t)(userLen & 0xFF));
        s_mqttClient.write((const uint8_t*)user, userLen);
    }
    // Pass
    if (flags & 0x40) {
        s_mqttClient.write((uint8_t)(passLen >> 8));
        s_mqttClient.write((uint8_t)(passLen & 0xFF));
        s_mqttClient.write((const uint8_t*)pass, passLen);
    }
}

static void mqttPublish(const char* topic, const char* payload, bool retain = false) {
    if (!s_mqttClient.connected()) return;

    uint16_t topicLen = strlen(topic);
    uint32_t payloadLen = strlen(payload);
    size_t remLen = 2 + topicLen + payloadLen;

    uint8_t header = 0x30 | (retain ? 0x01 : 0x00);
    s_mqttClient.write(header);

    // Variable length encoding for remaining length
    do {
        uint8_t digit = remLen % 128;
        remLen /= 128;
        if (remLen > 0) digit |= 0x80;
        s_mqttClient.write(digit);
    } while (remLen > 0);

    // Topic
    s_mqttClient.write((uint8_t)(topicLen >> 8));
    s_mqttClient.write((uint8_t)(topicLen & 0xFF));
    s_mqttClient.write((const uint8_t*)topic, topicLen);

    // Payload
    s_mqttClient.write((const uint8_t*)payload, payloadLen);
}

static void mqttSubscribe(const char* topic) {
    if (!s_mqttClient.connected()) return;

    uint16_t topicLen = strlen(topic);
    size_t remLen = 2 + 2 + topicLen + 1; // Packet ID + topic len + topic + QoS

    s_mqttClient.write((uint8_t)0x82); // SUBSCRIBE
    s_mqttClient.write((uint8_t)remLen);
    s_mqttClient.write((uint8_t)0x00); // Packet ID MSB
    s_mqttClient.write((uint8_t)0x01); // Packet ID LSB

    s_mqttClient.write((uint8_t)(topicLen >> 8));
    s_mqttClient.write((uint8_t)(topicLen & 0xFF));
    s_mqttClient.write((const uint8_t*)topic, topicLen);
    s_mqttClient.write((uint8_t)0x00); // QoS 0
}

static void mqttPing() {
    if (s_mqttClient.connected()) {
        uint8_t pingreq[] = { 0xC0, 0x00 };
        s_mqttClient.write(pingreq, 2);
    }
}

static void publishDiscovery() {
    state::lock();
    const char* prefix = state::config().mqttTopicPrefix;
    state::unlock();

    char stateTopic[64];
    snprintf(stateTopic, sizeof(stateTopic), "%s/state", prefix);

    auto publishSensor = [&](const char* id, const char* name, const char* unit, const char* devClass, const char* valTemplate) {
        char discTopic[128];
        snprintf(discTopic, sizeof(discTopic), "homeassistant/sensor/%s_%s/config", prefix, id);

        JsonDocument doc;
        doc["name"]        = name;
        doc["state_topic"] = stateTopic;
        doc["value_template"] = valTemplate;
        doc["unique_id"]   = String(prefix) + "_" + id;
        if (unit)     doc["unit_of_measurement"] = unit;
        if (devClass) doc["device_class"]         = devClass;

        JsonObject dev = doc["device"].to<JsonObject>();
        dev["identifiers"][0] = prefix;
        dev["name"]           = "MeteorS3 Weather Station";
        dev["model"]          = "ESP32-S3-LCD-2";
        dev["manufacturer"]   = "Waveshare";

        String out;
        serializeJson(doc, out);
        mqttPublish(discTopic, out.c_str(), true);
    };

    publishSensor("temp", "Temperature", "°C", "temperature", "{{ value_json.temperature }}");
    publishSensor("humidity", "Humidity", "%", "humidity", "{{ value_json.humidity }}");
    publishSensor("pressure", "Pressure", "hPa", "atmospheric_pressure", "{{ value_json.pressure }}");
    publishSensor("wind", "Wind Speed", "km/h", "wind_speed", "{{ value_json.wind }}");
    publishSensor("aqi", "Air Quality Index", "AQI", "aqi", "{{ value_json.aqi }}");
    publishSensor("battery", "Battery", "%", "battery", "{{ value_json.battery }}");
}

static void publishState() {
    state::lock();
    const WeatherData& w = state::weather();
    const AirData&     a = state::air();
    const char* prefix   = state::config().mqttTopicPrefix;
    state::unlock();

    char stateTopic[64];
    snprintf(stateTopic, sizeof(stateTopic), "%s/state", prefix);

    JsonDocument doc;
    if (w.valid) {
        doc["temperature"] = w.temperature;
        doc["apparent"]    = w.apparent;
        doc["humidity"]    = w.humidity;
        doc["pressure"]    = w.pressure;
        doc["wind"]        = w.wind;
        doc["gust"]        = w.gust;
        doc["uv"]          = w.uv;
        doc["weather_code"]= w.weatherCode;
    }
    if (a.valid) {
        doc["aqi"]         = a.usAqi;
        doc["pm25"]        = a.pm25;
        doc["pm10"]        = a.pm10;
    }
    doc["battery"]         = hal::readBatteryPercent();
    doc["battery_voltage"] = hal::readBatteryVoltage();
    doc["rssi"]            = WiFi.RSSI();
    doc["free_heap"]       = ESP.getFreeHeap();

    String json;
    serializeJson(doc, json);
    mqttPublish(stateTopic, json.c_str(), false);
}

void mqttInit() {
    s_connected = false;
}

void mqttService() {
    state::lock();
    bool enabled = state::config().mqttEnabled;
    const char* srv = state::config().mqttServer;
    uint16_t port   = state::config().mqttPort;
    const char* usr = state::config().mqttUser;
    const char* pwd = state::config().mqttPassword;
    const char* pfx = state::config().mqttTopicPrefix;
    state::unlock();

    if (!enabled || strlen(srv) == 0 || !isWifiConnected()) {
        if (s_mqttClient.connected()) s_mqttClient.stop();
        s_connected = false;
        return;
    }

    uint32_t now = millis();

    // 1. Connection check / Reconnect
    if (!s_mqttClient.connected()) {
        s_connected = false;
        if (now - s_lastConnectTry >= 10000) {
            s_lastConnectTry = now;
            Serial.printf("[mqtt] connecting to %s:%d...\n", srv, port);
            if (s_mqttClient.connect(srv, port, 5000)) {
                mqttConnect(pfx, usr, pwd);
                delay(200);

                if (s_mqttClient.connected()) {
                    s_connected = true;
                    Serial.println("[mqtt] connected!");

                    // Subscribe to custom alert topic
                    char alertTopic[64];
                    snprintf(alertTopic, sizeof(alertTopic), "%s/alert/set", pfx);
                    mqttSubscribe(alertTopic);

                    // Publish HA discovery
                    publishDiscovery();
                    publishState();
                    s_lastPublishMs = now;
                    s_lastPingMs    = now;
                }
            }
        }
        return;
    }

    // 2. Process incoming packets
    while (s_mqttClient.available()) {
        uint8_t header = s_mqttClient.read();
        uint8_t msgType = header >> 4;
        if (msgType == 3) { // PUBLISH
            // Read remaining length
            uint8_t remLen = s_mqttClient.read();
            if (remLen > 2) {
                uint8_t tLenMsb = s_mqttClient.read();
                uint8_t tLenLsb = s_mqttClient.read();
                uint16_t tLen = (tLenMsb << 8) | tLenLsb;

                char topicBuf[64];
                size_t readLen = min((size_t)tLen, sizeof(topicBuf) - 1);
                s_mqttClient.readBytes(topicBuf, readLen);
                topicBuf[readLen] = '\0';

                int pLen = remLen - 2 - tLen;
                if (pLen > 0) {
                    char payloadBuf[64];
                    size_t pReadLen = min((size_t)pLen, sizeof(payloadBuf) - 1);
                    s_mqttClient.readBytes(payloadBuf, pReadLen);
                    payloadBuf[pReadLen] = '\0';

                    Serial.printf("[mqtt] received alert: %s\n", payloadBuf);
                    state::setCustomAlert(payloadBuf);
                }
            }
        }
    }

    // 3. Periodic Keepalive Ping (every 30s)
    if (now - s_lastPingMs >= 30000) {
        s_lastPingMs = now;
        mqttPing();
    }

    // 4. Periodic State Publish (every 60s)
    if (now - s_lastPublishMs >= 60000) {
        s_lastPublishMs = now;
        publishState();
    }
}

bool isMqttConnected() {
    return s_connected;
}

}  // namespace svc
