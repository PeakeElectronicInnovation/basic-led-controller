#pragma once

// LED Configuration
#define LED_PIN     13
#define NUM_LEDS    120
#define LED_TYPE    WS2811
#define COLOR_ORDER BRG

// Status LED Configuration
#define WIFI_STATUS_LED_PIN  14
#define MQTT_STATUS_LED_PIN  4

// WiFi AP Configuration
#define AP_SSID     "LED-Controller"
#define AP_PASSWORD "ledcontrol123"

// Default MQTT Configuration
#define DEFAULT_MQTT_HOST   "homeassistant.local"
#define DEFAULT_MQTT_PORT   1883
#define DEFAULT_MQTT_USER   "user"
#define DEFAULT_MQTT_PASS   "pass"     // loginDev2024!

// Device Information
#define DEFAULT_HOSTNAME "led-planter"
#define DEVICE_NAME "LED Planter"
#define DEVICE_ID   "led_planter"
#define MQTT_BASE_TOPIC "homeassistant/light/led_planter"

// Web server port
#define HTTP_PORT   80

// EEPROM Configuration
#define EEPROM_SIZE 1024
#define WIFI_SSID_ADDR 0
#define WIFI_PASS_ADDR 32
#define MQTT_HOST_ADDR 96
#define MQTT_PORT_ADDR 160
#define MQTT_USER_ADDR 164
#define MQTT_PASS_ADDR 196
#define HOSTNAME_ADDR 228
#define SETTINGS_ADDR 292

// Settings magic number to verify EEPROM data
#define SETTINGS_MAGIC 0xAB54

// Settings structure
struct Settings {
    uint16_t magic;      // Magic number to verify settings
    uint8_t brightness;  // Global brightness (0-255)
    uint8_t colorR;      // Red component
    uint8_t colorG;      // Green component
    uint8_t colorB;      // Blue component
    char effect[32];    // Current effect name
    uint32_t lastWrite; // Timestamp of last write
};
