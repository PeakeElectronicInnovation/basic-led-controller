#pragma once
#include <FastLED.h>

class Effects {
private:
    CRGB* leds;
    int numLeds;
    uint8_t hue = 0;
    
    // Water effect parameters
    static const int MAX_RIPPLES = 3;  // Reduced number of ripples for smaller strip
    static const int LED_GROUP_SIZE = 7;  // Number of LEDs in each group
    struct Ripple {
        int center;         // Center point of the ripple
        int life;          // Current life of the ripple
        int maxLife;       // Maximum life of this ripple
        float amplitude;    // Height of the ripple
        float speed;       // Speed of ripple expansion
        bool active;       // Whether this ripple is currently active
    };
    Ripple ripples[MAX_RIPPLES];
    
    // Effect state variables
    uint8_t wavePosition = 0;
    uint8_t twinkleDimming = 40;

public:
    Effects(CRGB* ledArray, int numLeds) : leds(ledArray), numLeds(numLeds) {
        // Initialize ripples as inactive
        for (int i = 0; i < MAX_RIPPLES; i++) {
            ripples[i].active = false;
        }
    }
    
    void rainbow() {
        fill_rainbow(leds, numLeds, hue++, 7);
        FastLED.show();
        FastLED.delay(20);
    }
    
    void solid(CRGB color) {
        fill_solid(leds, numLeds, color);
        FastLED.show();
    }
    
    void twinkle(CRGB color) {
        // Fade existing pixels
        for (int i = 0; i < numLeds; i++) {
            leds[i].fadeToBlackBy(twinkleDimming);
        }
        
        // Randomly light up new pixels in groups
        if (random8() < 50) {
            int groupStart = random16(numLeds / LED_GROUP_SIZE) * LED_GROUP_SIZE;
            for (int i = 0; i < LED_GROUP_SIZE && groupStart + i < numLeds; i++) {
                leds[groupStart + i] = color;
            }
        }
        
        FastLED.show();
        FastLED.delay(20);
    }
    
    void colorWave(CRGB color) {
        for (int i = 0; i < numLeds; i++) {
            // Create smooth sine wave brightness
            uint8_t brightness = sin8(wavePosition + (i * 255 / numLeds));
            leds[i] = color;
            leds[i].nscale8(brightness);
        }
        wavePosition += 2;
        
        FastLED.show();
        FastLED.delay(20);
    }
    
    void ripple(CRGB color) {
        // Create base water color (slightly darker version of the selected color)
        CRGB baseColor = color;
        baseColor.nscale8(128);  // Reduce brightness for base
        
        // Fill with base color
        fill_solid(leds, numLeds, baseColor);
        
        // Update existing ripples
        for (int i = 0; i < MAX_RIPPLES; i++) {
            if (ripples[i].active) {
                // Calculate ripple spread
                for (int led = 0; led < numLeds; led++) {
                    float distance = abs(led - ripples[i].center);
                    float ripplePos = distance - (ripples[i].life * ripples[i].speed);
                    
                    // Create sine wave effect with wider spread
                    if (ripplePos >= 0 && ripplePos < LED_GROUP_SIZE * 2) {  // Wider spread
                        float brightness = sin(ripplePos * PI / (LED_GROUP_SIZE * 2)) * ripples[i].amplitude;
                        brightness = constrain(brightness, 0, 1);
                        
                        // Add highlight to base color
                        CRGB highlightColor = color;
                        highlightColor.nscale8(255 * brightness);
                        
                        // Apply the same highlight to the LED group
                        int groupStart = (led / LED_GROUP_SIZE) * LED_GROUP_SIZE;
                        for (int j = 0; j < LED_GROUP_SIZE && groupStart + j < numLeds; j++) {
                            leds[groupStart + j] += highlightColor;
                        }
                    }
                }
                
                // Update ripple life
                ripples[i].life++;
                if (ripples[i].life >= ripples[i].maxLife) {
                    ripples[i].active = false;
                }
            }
        }
        
        // Randomly create new ripples
        if (random8() < 25) {  // Reduced probability of new ripples
            for (int i = 0; i < MAX_RIPPLES; i++) {
                if (!ripples[i].active) {
                    // Initialize new ripple with random parameters
                    int groupNum = random16(numLeds / LED_GROUP_SIZE);
                    ripples[i].center = groupNum * LED_GROUP_SIZE + (LED_GROUP_SIZE / 2);
                    ripples[i].life = 0;
                    ripples[i].maxLife = random8(30, 50);  // Longer lifetime
                    ripples[i].amplitude = 0.3 + (random8() / 255.0) * 0.5;  // Reduced maximum amplitude
                    ripples[i].speed = 0.15 + (random8() / 255.0) * 0.2;  // Slower speed
                    ripples[i].active = true;
                    break;
                }
            }
        }
        
        // Apply gentle noise to simulate small surface variations
        for (int i = 0; i < numLeds; i += LED_GROUP_SIZE) {
            int8_t noise = random8(10) - 5;  // Reduced noise range
            for (int j = 0; j < LED_GROUP_SIZE && i + j < numLeds; j++) {
                leds[i + j].addToRGB(noise);
            }
        }
        
        FastLED.show();
        FastLED.delay(50);  // Slower animation speed
    }
};
