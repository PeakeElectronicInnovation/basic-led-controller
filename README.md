# LED Controller for WS2811 Strips

A sophisticated LED controller project designed for WS2811 LED strips, specifically optimized for a feature planter installation. Built on the ESP-WROOM-32 devkit module using the Arduino framework in PlatformIO.

## Features

- Controls WS2811 LED strips (supports up to 1000 LEDs per strip)
- WiFi-enabled web interface for remote control
- MQTT integration with Home Assistant
- Easy network configuration through AP mode
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
