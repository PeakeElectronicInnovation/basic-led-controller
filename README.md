# LED Controller for WS2811 Strips

A sophisticated LED controller project designed for WS2811 LED strips, specifically optimized for a feature planter installation. Built on the ESP-WROOM-32 devkit module using the Arduino framework in PlatformIO.

## Features

- Controls WS2811 LED strips (supports up to 1000 LEDs per strip)
- WiFi-enabled web interface for remote control
- MQTT integration with Home Assistant
- Easy network configuration through AP mode
- Over-The-Air (OTA) firmware updates
- Multiple lighting effects including:
  - Basic color control
  - Brightness adjustment
  - Water ripple effect
  - Various other visual effects

## Hardware Requirements

- ESP-WROOM-32 devkit module
- WS2811 LED strip
- Power supply (appropriate for your LED strip length)
- USB cable for programming

## Software Dependencies

- PlatformIO
- Libraries:
  - FastLED
  - ESPAsyncWebServer
  - AsyncMqttClient
  - ArduinoJson
  - WiFi
  - AsyncElegantOTA

## Installation

1. Clone this repository
2. Open the project in PlatformIO
3. Install the required dependencies
4. Upload the code to your ESP32 device

## Initial Setup

1. Power on the device
2. Connect to the ESP32's AP mode WiFi network
3. Configure your home WiFi credentials through the web interface
4. (Optional) Configure MQTT settings for Home Assistant integration
5. The device will restart and connect to your network

## Usage

### Web Interface
- Access the web interface through your browser using the device's IP address
- Control colors, brightness, and effects
- Configure device settings
- Update firmware via the `/update` endpoint

### Firmware Updates
1. Access the OTA update interface by navigating to `http://<device-ip>/update`
2. Select the new firmware file (.bin)
3. Click "Update" to begin the process
4. The device will restart automatically after the update

### Home Assistant Integration
- The device supports MQTT auto-discovery
- Automatically appears in Home Assistant when properly configured
- Control through Home Assistant interface or automations

## Project Structure

- `/src` - Main source code
- `/include` - Header files
- `/lib` - Project-specific libraries
- `/test` - Test files
- `platformio.ini` - PlatformIO configuration

## Contributing

Feel free to submit issues and enhancement requests!

## License

This project is open source and available under the MIT License.

## Author

Jeremy Peake

## Version

1.0

# LED Controller

A WiFi-enabled LED controller for addressable LED strips, built with ESP32 and FastLED.

## Features

- Multiple LED effects and animations
- Web interface for control and configuration
- MQTT integration with Home Assistant
- Over-The-Air (OTA) firmware updates
- State persistence across reboots
- Real-time effect parameter adjustment

## Getting Started

### Hardware Requirements

- ESP32 development board
- WS2812B or similar addressable LED strip
- 5V power supply (sized appropriately for your LED strip)
- Level shifter (recommended for reliable operation)

### Software Requirements

- PlatformIO
- Required libraries (see platformio.ini)

### Installation

1. Clone this repository
2. Open in PlatformIO
3. Update `config.h` with your WiFi and MQTT credentials
4. Build and upload to your ESP32

### Updating Firmware

The LED Controller supports Over-The-Air (OTA) updates for easy firmware upgrades:

1. Connect to your LED Controller's IP address
2. Navigate to `http://<device-ip>/update`
3. Select the new firmware file (.bin)
4. Click "Update Firmware"
5. Wait for the upload to complete and the device to restart

## Web Interface

The web interface is available at `http://<device-ip>/` and provides:

- Effect selection and configuration
- Color picker
- Brightness control
- Power control
- Firmware updates (via `/update` endpoint)

## MQTT Integration

The controller integrates with Home Assistant via MQTT, providing:

- Automatic discovery
- Power control
- Brightness control
- Effect selection
- Color control

## Project Structure

- `src/main.cpp`: Main application code
- `src/effects.h`: LED effect implementations
- `src/config.h`: Configuration settings
- `src/web_interface.h`: Web interface HTML and JavaScript

## Contributing

Feel free to submit issues and pull requests.

## License

This project is licensed under the MIT License - see the LICENSE file for details.
