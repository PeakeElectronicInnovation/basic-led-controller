#pragma once
#include <EEPROM.h>
#include "config.h"

class SettingsManager {
private:
    Settings settings;
    static const uint32_t MIN_WRITE_INTERVAL = 5000;  // Minimum time between writes (5 seconds)
    bool initialized = false;

    void dumpSettings(const char* prefix) {
        Serial.printf("%s:\n", prefix);
        Serial.printf("  Magic: 0x%04X (Expected: 0x%04X)\n", settings.magic, SETTINGS_MAGIC);
        Serial.printf("  Brightness: %d\n", settings.brightness);
        Serial.printf("  Color: R:%d G:%d B:%d\n", settings.colorR, settings.colorG, settings.colorB);
        Serial.printf("  Effect: %s\n", settings.effect);
        Serial.printf("  LastWrite: %lu\n", settings.lastWrite);
    }

public:
    SettingsManager() {
        // Don't load settings in constructor - wait for begin() call
    }

    void begin() {
        if (!initialized) {
            loadSettings();
            initialized = true;
        }
    }

    void loadSettings() {
        Serial.println("Reading settings from EEPROM...");
        
        // Read the current settings
        Settings tempSettings;
        EEPROM.get(SETTINGS_ADDR, tempSettings);
        
        // Check if settings are valid
        if (tempSettings.magic != SETTINGS_MAGIC) {
            Serial.println("Invalid settings detected, initializing defaults...");
            
            // Initialize with defaults
            settings.magic = SETTINGS_MAGIC;
            settings.brightness = 255;
            settings.colorR = 255;  // Default to white
            settings.colorG = 255;
            settings.colorB = 255;
            strcpy(settings.effect, "solid");
            settings.lastWrite = 0;
            
            // Force immediate save of defaults
            Serial.println("Saving default settings...");
            EEPROM.put(SETTINGS_ADDR, settings);
            if (EEPROM.commit()) {
                Serial.println("Default settings saved successfully");
            } else {
                Serial.println("Error saving default settings!");
            }
        } else {
            Serial.println("Valid settings found, loading...");
            settings = tempSettings;
        }
        
        dumpSettings("Current settings");
    }

    void saveSettings(bool force = false) {
        if (!initialized) {
            Serial.println("Warning: Attempted to save settings before initialization");
            return;
        }
        
        uint32_t now = millis();
        
        // Only write if forced or enough time has passed since last write
        if (force || (now - settings.lastWrite >= MIN_WRITE_INTERVAL)) {
            Serial.println("Saving settings to EEPROM...");
            dumpSettings("Settings to save");
            
            settings.lastWrite = now;
            settings.magic = SETTINGS_MAGIC;  // Ensure magic is set
            
            // Write settings and ensure they are committed
            EEPROM.put(SETTINGS_ADDR, settings);
            bool success = EEPROM.commit();
            
            if (success) {
                Serial.println("Settings saved successfully");
                
                // Verify the save
                Settings verify;
                EEPROM.get(SETTINGS_ADDR, verify);
                if (verify.magic != SETTINGS_MAGIC ||
                    verify.brightness != settings.brightness ||
                    verify.colorR != settings.colorR ||
                    verify.colorG != settings.colorG ||
                    verify.colorB != settings.colorB ||
                    strcmp(verify.effect, settings.effect) != 0) {
                    Serial.println("Warning: Settings verification failed!");
                    dumpSettings("Verification read");
                }
            } else {
                Serial.println("Error saving settings!");
            }
        }
    }

    // Getters
    uint8_t getBrightness() {
        return settings.brightness;
    }

    CRGB getColor() {
        return CRGB(settings.colorR, settings.colorG, settings.colorB);
    }

    const char* getEffect() {
        return settings.effect;
    }

    // Setters with immediate save option
    void setBrightness(uint8_t value, bool immediate = false) {
        if (settings.brightness != value) {
            Serial.printf("Setting brightness to %d\n", value);
            settings.brightness = value;
            saveSettings(immediate);
        }
    }

    void setColor(CRGB color, bool immediate = false) {
        if (settings.colorR != color.r || 
            settings.colorG != color.g || 
            settings.colorB != color.b) {
            Serial.printf("Setting color to R:%d G:%d B:%d\n", color.r, color.g, color.b);
            settings.colorR = color.r;
            settings.colorG = color.g;
            settings.colorB = color.b;
            saveSettings(immediate);
        }
    }

    void setEffect(const char* effect, bool immediate = false) {
        if (strcmp(settings.effect, effect) != 0) {
            Serial.printf("Setting effect to %s\n", effect);
            strncpy(settings.effect, effect, sizeof(settings.effect) - 1);
            settings.effect[sizeof(settings.effect) - 1] = '\0';
            saveSettings(immediate);
        }
    }
};
