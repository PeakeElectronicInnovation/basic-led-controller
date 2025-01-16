#pragma once
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include "config.h"

class MQTTHandler {
private:
    AsyncMqttClient* mqttClient;
    const char* baseTopic;
    
    // Callback function pointers
    void (*brightnessCallback)(uint8_t);
    void (*colorCallback)(uint32_t);
    void (*effectCallback)(const char*);
    
    void publishDiscovery() {
        StaticJsonDocument<1024> doc;
        
        doc["name"] = DEVICE_NAME;
        doc["unique_id"] = DEVICE_ID;
        doc["cmd_t"] = "~/set";
        doc["stat_t"] = "~/state";
        doc["schema"] = "json";
        doc["brightness"] = true;
        doc["rgb"] = true;
        doc["effect"] = true;
        
        JsonArray effect_list = doc.createNestedArray("effect_list");
        effect_list.add("solid");
        effect_list.add("rainbow");
        effect_list.add("ripple");
        effect_list.add("twinkle");
        effect_list.add("wave");
        
        char discovery_topic[128];
        snprintf(discovery_topic, sizeof(discovery_topic), 
                "homeassistant/light/%s/config", DEVICE_ID);
        
        String output;
        serializeJson(doc, output);
        mqttClient->publish(discovery_topic, 0, true, output.c_str());
    }
    
    void handleCommand(const char* payload) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error) {
            return;
        }
        
        if (doc.containsKey("brightness")) {
            uint8_t brightness = doc["brightness"];
            if (brightnessCallback) brightnessCallback(brightness);
        }
        
        if (doc.containsKey("color")) {
            JsonObject color = doc["color"];
            uint32_t rgb = (uint32_t)color["r"] << 16 | 
                          (uint32_t)color["g"] << 8 | 
                          (uint32_t)color["b"];
            if (colorCallback) colorCallback(rgb);
        }
        
        if (doc.containsKey("effect")) {
            const char* effect = doc["effect"];
            if (effectCallback) effectCallback(effect);
        }
    }
    
public:
    MQTTHandler(AsyncMqttClient* client, const char* topic) : 
        mqttClient(client), baseTopic(topic) {
        
        // Setup MQTT callbacks
        client->onConnect([this](bool sessionPresent) {
            char cmdTopic[64];
            snprintf(cmdTopic, sizeof(cmdTopic), "%s/set", baseTopic);
            mqttClient->subscribe(cmdTopic, 0);
            publishDiscovery();
        });
        
        client->onMessage([this](char* topic, char* payload, 
            AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
            
            if (strstr(topic, "/set")) {
                char* payloadCopy = (char*)malloc(len + 1);
                memcpy(payloadCopy, payload, len);
                payloadCopy[len] = '\0';
                handleCommand(payloadCopy);
                free(payloadCopy);
            }
        });
    }
    
    void begin() {
        mqttClient->connect();
    }
    
    void publishState(uint8_t brightness, uint32_t color, const char* effect) {
        StaticJsonDocument<200> doc;
        
        doc["state"] = "ON";
        doc["brightness"] = brightness;
        
        JsonObject colorObj = doc.createNestedObject("color");
        colorObj["r"] = (color >> 16) & 0xFF;
        colorObj["g"] = (color >> 8) & 0xFF;
        colorObj["b"] = color & 0xFF;
        
        doc["effect"] = effect;
        
        char stateTopic[64];
        snprintf(stateTopic, sizeof(stateTopic), "%s/state", baseTopic);
        
        String output;
        serializeJson(doc, output);
        mqttClient->publish(stateTopic, 0, true, output.c_str());
    }
    
    void setCallbacks(void (*brightness)(uint8_t), 
                     void (*color)(uint32_t),
                     void (*effect)(const char*)) {
        brightnessCallback = brightness;
        colorCallback = color;
        effectCallback = effect;
    }
};
