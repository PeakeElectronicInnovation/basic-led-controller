#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <Update.h>
#include "config.h"
#include "web_interface.h"
#include "effects.h"
#include "mqtt_handler.h"
#include "settings_manager.h"

// LED strip configuration
CRGB leds[NUM_LEDS];
uint8_t brightness = 255;
CRGB currentColor = CRGB::White;
String currentEffect = "solid";

// WiFi credentials
String ssid;
String password;

// Global variables for MQTT settings
String mqtt_host;
uint16_t mqtt_port;
String mqtt_user;
String mqtt_pass;

// Global variables
String hostname;
uint8_t wifiConnectionAttempts = 0;
const uint8_t MAX_WIFI_ATTEMPTS = 3;

// Create objects
AsyncWebServer server(80);
AsyncMqttClient mqttClient;
Effects* effects;
SettingsManager settingsManager;
MQTTHandler* mqtt;

// HTML for update page
const char* updateHTML = R"(
<!DOCTYPE html>
<html>
<head>
    <title>LED Controller Update</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            background: #f0f0f0;
        }
        .container {
            background: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            max-width: 500px;
            margin: 0 auto;
        }
        h2 {
            color: #333;
            margin-bottom: 20px;
        }
        .info {
            margin-bottom: 20px;
            padding: 10px;
            background: #e8f4f8;
            border-radius: 4px;
        }
        form {
            margin-top: 20px;
        }
        input[type="file"] {
            display: block;
            margin: 10px 0;
            width: 100%;
        }
        input[type="submit"] {
            background: #4CAF50;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
        }
        input[type="submit"]:hover {
            background: #45a049;
        }
        #progress {
            margin-top: 20px;
            display: none;
        }
        .progress-bar {
            height: 20px;
            background: #f0f0f0;
            border-radius: 10px;
            overflow: hidden;
        }
        .progress-fill {
            height: 100%;
            background: #4CAF50;
            width: 0%;
            transition: width 0.3s;
        }
    </style>
</head>
<body>
    <div class="container">
        <h2>LED Controller Firmware Update</h2>
        <div class="info">
            Current Version: %VERSION%<br>
            Free Space: %FREESPACE% bytes
        </div>
        <form method='POST' action='/update' enctype='multipart/form-data' id='upload_form'>
            <input type='file' name='update' accept='.bin'>
            <input type='submit' value='Update Firmware'>
        </form>
        <div id="progress">
            <p>Upload Progress: <span id="percent">0%</span></p>
            <div class="progress-bar">
                <div class="progress-fill" id="bar"></div>
            </div>
        </div>
    </div>
    <script>
        var form = document.getElementById('upload_form');
        var progress = document.getElementById('progress');
        var percent = document.getElementById('percent');
        var bar = document.getElementById('bar');

        form.onsubmit = function(e) {
            e.preventDefault();
            var data = new FormData(form);
            var xhr = new XMLHttpRequest();
            xhr.open('POST', '/update', true);
            
            // Show progress bar
            progress.style.display = 'block';
            
            xhr.upload.onprogress = function(e) {
                if (e.lengthComputable) {
                    var percentComplete = (e.loaded / e.total) * 100;
                    percent.textContent = percentComplete.toFixed(2) + '%';
                    bar.style.width = percentComplete + '%';
                }
            };
            
            xhr.onload = function() {
                if (xhr.status === 200) {
                    alert('Update successful! Device will restart.');
                } else {
                    alert('Update failed!');
                }
            };
            
            xhr.send(data);
        };
    </script>
</body>
</html>
)";

void handleUpdate(AsyncWebServerRequest *request) {
    String html = String(updateHTML);
    html.replace("%VERSION%", FIRMWARE_VERSION);
    html.replace("%FREESPACE%", String(ESP.getFreeSketchSpace()));
    request->send(200, "text/html", html);
}

void handleDoUpdate(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
        Serial.println("Update started");
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
            return request->send(400, "text/plain", "OTA update failed");
        }
    }

    if (Update.write(data, len) != len) {
        Update.printError(Serial);
        return request->send(400, "text/plain", "OTA update failed");
    }

    if (final) {
        if (!Update.end(true)) {
            Update.printError(Serial);
            return request->send(400, "text/plain", "OTA update failed");
        }
        Serial.println("Update complete");
        request->send(200, "text/plain", "Update successful. Device will restart.");
        delay(1000);
        ESP.restart();
    }
}

// Function declarations
void setupWiFi();
void setupWebServer();
void setupMQTT();
void handleWiFiSetup(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
void applyEffect(String effect);
void publishState();
void handleRequests();
void loadMQTTSettings();
void handleMQTTSetup(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
void handleGetSettings(AsyncWebServerRequest *request);
void handleGetState(AsyncWebServerRequest *request);
void loadHostname();
void handleHostnameSetup(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);

// MQTT callbacks
void onMqttBrightness(uint8_t value) {
    brightness = value;
    FastLED.setBrightness(brightness);
}

void onMqttColor(uint32_t color) {
    currentColor = CRGB((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
    if (currentEffect == "solid") {
        effects->solid(currentColor);
    }
}

void onMqttEffect(const char* effect) {
    currentEffect = effect;
    applyEffect(currentEffect);
}

void onMqttConnect(bool sessionPresent) {
    Serial.println("Connected to MQTT.");
    digitalWrite(MQTT_STATUS_LED_PIN, HIGH);  // Turn on MQTT status LED
    
    // Subscribe to topics
    uint16_t packetIdSub = mqttClient.subscribe(MQTT_BASE_TOPIC "/set", 2);
    Serial.print("Subscribing at QoS 2, packetId: ");
    Serial.println(packetIdSub);
    
    // Publish initial state
    publishState();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    Serial.println("Disconnected from MQTT.");
    digitalWrite(MQTT_STATUS_LED_PIN, LOW);  // Turn off MQTT status LED
}

void startAPMode() {
    Serial.println("Starting AP Mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    digitalWrite(WIFI_STATUS_LED_PIN, HIGH);  // Turn on WiFi LED in AP mode
}

void WiFiEvent(WiFiEvent_t event) {
    Serial.printf("WiFi event: %d\n", event);
    switch(event) {
        case SYSTEM_EVENT_STA_START:
            Serial.println("WiFi client started");
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("Connected to WiFi access point");
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.println("WiFi connected");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
            wifiConnectionAttempts = 0;  // Reset attempt counter on successful connection
            digitalWrite(WIFI_STATUS_LED_PIN, HIGH);  // Turn on WiFi status LED
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("WiFi lost connection");
            digitalWrite(WIFI_STATUS_LED_PIN, LOW);  // Turn off WiFi status LED
            
            if (++wifiConnectionAttempts < MAX_WIFI_ATTEMPTS) {
                Serial.printf("Retrying connection, attempt %d/%d\n", 
                    wifiConnectionAttempts, MAX_WIFI_ATTEMPTS);
                WiFi.begin();  // Retry connection
            } else {
                Serial.println("Maximum connection attempts reached");
                startAPMode();  // Switch to AP mode
            }
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n\nStarting up...");
    
    // Initialize status LEDs
    pinMode(WIFI_STATUS_LED_PIN, OUTPUT);
    pinMode(MQTT_STATUS_LED_PIN, OUTPUT);
    digitalWrite(WIFI_STATUS_LED_PIN, LOW);
    digitalWrite(MQTT_STATUS_LED_PIN, LOW);
    
    // Initialize EEPROM
    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("Failed to initialize EEPROM!");
        delay(1000);
        ESP.restart();
    }

    // Initialize LED strip
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(brightness);
    effects = new Effects(leds, NUM_LEDS);

    // Load saved settings
    loadHostname();
    loadMQTTSettings();
    
    // Set up WiFi event handlers and start WiFi
    WiFi.onEvent(WiFiEvent);
    setupWiFi();
    
    // Wait for WiFi connection before proceeding with MQTT setup
    uint8_t wifiWaitAttempts = 0;
    while (WiFi.status() != WL_CONNECTED && wifiWaitAttempts < 20) {
        delay(500);
        wifiWaitAttempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        setupMQTT();
        mqttClient.connect();
    }
    
    // Set up web server (this works in both AP and STA modes)
    setupWebServer();
}

void publishState() {
    StaticJsonDocument<200> doc;
    doc["state"] = (brightness > 0) ? "ON" : "OFF";
    doc["brightness"] = brightness;
    
    JsonObject color = doc.createNestedObject("color");
    // Use raw color values for state reporting
    color["r"] = currentColor.r;
    color["g"] = currentColor.g;
    color["b"] = currentColor.b;
    
    if (currentEffect.length() > 0) {
        doc["effect"] = currentEffect;
    }
    
    String output;
    serializeJson(doc, output);
    
    char state_topic[64];
    snprintf(state_topic, sizeof(state_topic), "%s/state", MQTT_BASE_TOPIC);
    mqttClient.publish(state_topic, 0, true, output.c_str());
}

void setupWiFi() {
    // Register WiFi event handler first
    WiFi.onEvent(WiFiEvent);
    
    // Load WiFi credentials from EEPROM
    char ssid[32] = {0};  // Initialize with zeros
    char password[64] = {0};  // Initialize with zeros
    char hostname[32] = {0};  // Initialize with zeros
    
    // Read credentials from EEPROM
    EEPROM.get(WIFI_SSID_ADDR, ssid);
    EEPROM.get(WIFI_PASS_ADDR, password);
    EEPROM.get(HOSTNAME_ADDR, hostname);
    
    // Ensure null termination
    ssid[31] = '\0';
    password[63] = '\0';
    hostname[31] = '\0';
    
    // Set hostname (use default if empty)
    if (strlen(hostname) == 0) {
        strcpy(hostname, DEFAULT_HOSTNAME);
    }
    WiFi.setHostname(hostname);
    
    // Check if we have stored credentials
    if (strlen(ssid) > 0) {
        // Try to connect to stored WiFi
        Serial.printf("Attempting to connect to WiFi: %s\n", ssid);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
        wifiConnectionAttempts = 0;  // Reset attempt counter
    } else {
        // No stored credentials, start in AP mode immediately
        Serial.println("No stored WiFi credentials");
        startAPMode();
    }
}

void setupWebServer() {
    // Serve the web interface
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", index_html);
    });

    handleRequests();

    // Handle WiFi setup
    server.on("/setup-wifi", HTTP_POST, 
        [](AsyncWebServerRequest *request){},
        NULL,
        handleWiFiSetup
    );

    // Handle MQTT setup
    server.on("/setup-mqtt", HTTP_POST, 
        [](AsyncWebServerRequest *request){},
        NULL,
        handleMQTTSetup
    );
    
    // Handle hostname setup
    server.on("/setup-hostname", HTTP_POST, 
        [](AsyncWebServerRequest *request){},
        NULL,
        handleHostnameSetup
    );
    
    // Handle settings retrieval
    server.on("/get-settings", HTTP_GET, handleGetSettings);
    
    // Handle state retrieval
    server.on("/get-state", HTTP_GET, handleGetState);

    // Handle OTA Update
    server.on("/update", HTTP_GET, handleUpdate);
    server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200);
    }, handleDoUpdate);
    
    // Start server
    server.begin();
    Serial.println("HTTP server started");
}

void handleRequests() {
    server.on("/brightness", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("value")) {
            brightness = request->getParam("value")->value().toInt();
            FastLED.setBrightness(brightness);
            settingsManager.setBrightness(brightness, true);  // Save immediately
            publishState();
            request->send(200, "text/plain", "OK");
        }
    });

    server.on("/color", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("value")) {
            String colorStr = request->getParam("value")->value();
            uint32_t number = (uint32_t) strtol(colorStr.c_str(), NULL, 16);
            currentColor = CRGB(number >> 16, (number >> 8) & 0xFF, number & 0xFF);
            
            Serial.printf("Setting color to R:%d G:%d B:%d\n", currentColor.r, currentColor.g, currentColor.b);
            settingsManager.setColor(currentColor, true);  // Force immediate save
            
            if (currentEffect == "solid") {
                effects->solid(currentColor);
            }
            FastLED.show();
            publishState();
            request->send(200, "text/plain", "OK");
        }
    });

    server.on("/effect", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("name")) {
            currentEffect = request->getParam("name")->value();
            settingsManager.setEffect(currentEffect.c_str(), true);  // Save immediately
            applyEffect(currentEffect);
            publishState();
            request->send(200, "text/plain", "OK");
        }
    });
}

void handleGetState(AsyncWebServerRequest *request) {
    StaticJsonDocument<200> doc;
    doc["brightness"] = brightness;
    doc["effect"] = currentEffect;
    
    // Convert RGB values to hex color
    char colorHex[8];
    sprintf(colorHex, "#%02X%02X%02X", currentColor.r, currentColor.g, currentColor.b);
    doc["color"] = colorHex;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void handleWiFiSetup(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        String json = String((char*)data);
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, json);
        
        if (!error) {
            // Clear old credentials in EEPROM
            for (int i = WIFI_SSID_ADDR; i < WIFI_PASS_ADDR + 64; i++) {
                EEPROM.write(i, 0);
            }
            
            // Store new credentials
            String newSsid = doc["ssid"].as<String>();
            String newPassword = doc["password"].as<String>();
            
            // Validate SSID is not empty
            if (newSsid.length() == 0) {
                request->send(400, "text/plain", "SSID cannot be empty");
                return;
            }
            
            Serial.print("Saving new WiFi credentials - SSID: ");
            Serial.println(newSsid);
            
            // Write SSID
            for (size_t i = 0; i < newSsid.length(); i++) {
                EEPROM.write(WIFI_SSID_ADDR + i, newSsid[i]);
            }
            
            // Write Password
            for (size_t i = 0; i < newPassword.length(); i++) {
                EEPROM.write(WIFI_PASS_ADDR + i, newPassword[i]);
            }
            
            if (EEPROM.commit()) {
                request->send(200, "text/plain", "WiFi settings saved successfully. Rebooting...");
                delay(500);  // Give time for the response to be sent
                ESP.restart();
            } else {
                request->send(500, "text/plain", "Failed to save WiFi settings");
            }
            return;
        }
    }
    request->send(400, "text/plain", "Invalid request format");
}

void handleMQTTSetup(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        String json = String((char*)data);
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, json);
        
        if (!error) {
            // Clear old settings in EEPROM
            for (int i = MQTT_HOST_ADDR; i < MQTT_PASS_ADDR + 32; i++) {
                EEPROM.write(i, 0);
            }
            
            // Store new settings
            String newHost = doc["host"].as<String>();
            uint16_t newPort = doc["port"] | DEFAULT_MQTT_PORT;
            String newUser = doc["user"].as<String>();
            String newPass = doc["password"].as<String>();
            
            // Validate host is not empty
            if (newHost.length() == 0) {
                request->send(400, "text/plain", "MQTT host cannot be empty");
                return;
            }
            
            // Write host
            for (size_t i = 0; i < newHost.length(); i++) {
                EEPROM.write(MQTT_HOST_ADDR + i, newHost[i]);
            }
            
            // Write port
            EEPROM.write(MQTT_PORT_ADDR, newPort >> 8);
            EEPROM.write(MQTT_PORT_ADDR + 1, newPort & 0xFF);
            
            // Write username
            for (size_t i = 0; i < newUser.length(); i++) {
                EEPROM.write(MQTT_USER_ADDR + i, newUser[i]);
            }
            
            // Write password
            for (size_t i = 0; i < newPass.length(); i++) {
                EEPROM.write(MQTT_PASS_ADDR + i, newPass[i]);
            }
            
            if (EEPROM.commit()) {
                request->send(200, "text/plain", "MQTT settings saved successfully. Rebooting...");
                delay(500);  // Give time for the response to be sent
                ESP.restart();
            } else {
                request->send(500, "text/plain", "Failed to save MQTT settings");
            }
            return;
        }
    }
    request->send(400, "text/plain", "Invalid request format");
}

void handleGetSettings(AsyncWebServerRequest *request) {
    StaticJsonDocument<512> doc;
    JsonObject mqtt = doc.createNestedObject("mqtt");
    mqtt["host"] = mqtt_host;
    mqtt["port"] = mqtt_port;
    mqtt["user"] = mqtt_user;
    doc["hostname"] = hostname;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void handleHostnameSetup(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        String json = String((char*)data);
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, json);
        
        if (!error) {
            String newHostname = doc["hostname"].as<String>();
            
            // Validate hostname
            if (newHostname.length() == 0 || newHostname.length() > 32) {
                request->send(400, "text/plain", "Hostname must be between 1 and 32 characters");
                return;
            }
            
            // Clear old hostname in EEPROM
            for (int i = HOSTNAME_ADDR; i < HOSTNAME_ADDR + 32; i++) {
                EEPROM.write(i, 0);
            }
            
            // Write new hostname
            for (size_t i = 0; i < newHostname.length(); i++) {
                EEPROM.write(HOSTNAME_ADDR + i, newHostname[i]);
            }
            
            if (EEPROM.commit()) {
                request->send(200, "text/plain", "Hostname saved successfully. Rebooting...");
                delay(500);  // Give time for the response to be sent
                ESP.restart();
            } else {
                request->send(500, "text/plain", "Failed to save hostname");
            }
            return;
        }
    }
    request->send(400, "text/plain", "Invalid request format");
}

void handleWiFiConfig(AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
        char ssid[32] = {0};
        char password[64] = {0};
        
        // Get the new credentials
        String newSSID = request->getParam("ssid", true)->value();
        String newPassword = request->getParam("password", true)->value();
        
        // Copy with length limits and ensure null termination
        strncpy(ssid, newSSID.c_str(), 31);
        strncpy(password, newPassword.c_str(), 63);
        ssid[31] = '\0';
        password[63] = '\0';
        
        // Save to EEPROM
        EEPROM.put(WIFI_SSID_ADDR, ssid);
        EEPROM.put(WIFI_PASS_ADDR, password);
        
        if (EEPROM.commit()) {
            Serial.println("WiFi credentials saved");
            Serial.printf("SSID: %s\n", ssid);
            // Don't print password for security
            
            // Attempt to connect with new credentials
            WiFi.disconnect();
            delay(1000);
            WiFi.begin(ssid, password);
            wifiConnectionAttempts = 0;  // Reset attempt counter
            
            request->send(200, "text/plain", "WiFi credentials updated");
        } else {
            request->send(500, "text/plain", "Failed to save credentials");
        }
    } else {
        request->send(400, "text/plain", "Missing parameters");
    }
}

void loadMQTTSettings() {
    mqtt_host = "";
    mqtt_user = "";
    mqtt_pass = "";
    
    // Read MQTT host
    for (int i = MQTT_HOST_ADDR; i < MQTT_HOST_ADDR + 64; i++) {
        char c = EEPROM.read(i);
        if (c != 0) mqtt_host += c;
    }
    
    // Read MQTT port
    mqtt_port = (EEPROM.read(MQTT_PORT_ADDR) << 8) | EEPROM.read(MQTT_PORT_ADDR + 1);
    if (mqtt_port == 0xFFFF) mqtt_port = DEFAULT_MQTT_PORT;
    
    // Read MQTT username
    for (int i = MQTT_USER_ADDR; i < MQTT_USER_ADDR + 32; i++) {
        char c = EEPROM.read(i);
        if (c != 0) mqtt_user += c;
    }
    
    // Read MQTT password
    for (int i = MQTT_PASS_ADDR; i < MQTT_PASS_ADDR + 32; i++) {
        char c = EEPROM.read(i);
        if (c != 0) mqtt_pass += c;
    }
    
    // Use defaults if no values stored
    if (mqtt_host.length() == 0) mqtt_host = DEFAULT_MQTT_HOST;
    if (mqtt_user.length() == 0) mqtt_user = DEFAULT_MQTT_USER;
    if (mqtt_pass.length() == 0) mqtt_pass = DEFAULT_MQTT_PASS;
}

void loadHostname() {
    hostname = "";
    for (int i = HOSTNAME_ADDR; i < HOSTNAME_ADDR + 32; i++) {
        char c = EEPROM.read(i);
        if (c != 0) hostname += c;
    }
    
    if (hostname.length() == 0) {
        hostname = DEFAULT_HOSTNAME;
    }
}

void applyEffect(String effect) {
    if (effect == "solid") {
        effects->solid(currentColor);
    } else if (effect == "rainbow") {
        effects->rainbow();
    } else if (effect == "ripple") {
        effects->ripple(currentColor);
    } else if (effect == "twinkle") {
        effects->twinkle(currentColor);
    } else if (effect == "wave") {
        effects->colorWave(currentColor);
    }
}

void setupMQTT() {
    Serial.println("Setting up MQTT...");
    Serial.print("MQTT Host: ");
    Serial.println(mqtt_host);
    Serial.print("MQTT Port: ");
    Serial.println(mqtt_port);
    Serial.print("MQTT User: ");
    Serial.println(mqtt_user);
    
    mqttClient.setServer(mqtt_host.c_str(), mqtt_port);
    
    // Set credentials if provided
    if (strlen(mqtt_user.c_str()) > 0) {
        Serial.println("Using MQTT authentication");
        mqttClient.setCredentials(mqtt_user.c_str(), mqtt_pass.c_str());
    } else {
        Serial.println("No MQTT authentication");
    }
    
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    
    mqttClient.onMessage([](char* topic, char* payload, 
        AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
        
        String message = String(payload);
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, message);
        
        if (!error) {
            bool stateChanged = false;
            
            if (doc.containsKey("state")) {
                String state = doc["state"].as<String>();
                if (state == "ON") {
                    if (doc.containsKey("brightness")) {
                        brightness = doc["brightness"].as<int>();
                        settingsManager.setBrightness(brightness);
                        stateChanged = true;
                    }
                } else {
                    brightness = 0;
                    settingsManager.setBrightness(brightness);
                    stateChanged = true;
                }
            }
            
            if (doc.containsKey("brightness") && !doc.containsKey("state")) {
                brightness = doc["brightness"].as<int>();
                settingsManager.setBrightness(brightness);
                stateChanged = true;
            }
            
            if (doc.containsKey("color")) {
                JsonObject color = doc["color"];
                if (color.containsKey("r") && color.containsKey("g") && color.containsKey("b")) {
                    currentColor.r = color["r"].as<int>();
                    currentColor.g = color["g"].as<int>();
                    currentColor.b = color["b"].as<int>();
                    settingsManager.setColor(currentColor);
                    stateChanged = true;
                }
            }
            
            if (doc.containsKey("effect")) {
                currentEffect = doc["effect"].as<String>();
                settingsManager.setEffect(currentEffect.c_str());
                stateChanged = true;
            }
            
            if (stateChanged) {
                FastLED.setBrightness(brightness);
                if (currentEffect == "solid") {
                    fill_solid(leds, NUM_LEDS, currentColor);
                }
                FastLED.show();
                
                // Publish the current state back to Home Assistant
                publishState();
            }
        }
    });
}

void loop() {
    // Update LED animations based on current effect
    if (currentEffect == "solid") {
        effects->solid(currentColor);
    } else if (currentEffect == "rainbow") {
        effects->rainbow();
    } else if (currentEffect == "ripple") {
        effects->ripple(currentColor);
    } else if (currentEffect == "twinkle") {
        effects->twinkle(currentColor);
    } else if (currentEffect == "wave") {
        effects->colorWave(currentColor);
    }
    
    FastLED.show();
    delay(20);
}
