# ESP8266 IoT Mood Lamp

A smart ambient lighting solution built with ESP8266 that combines atmospheric RGB lighting with environmental monitoring. This IoT device features wireless connectivity, MQTT communication, and real-time sensor data transmission, making it perfect for smart home integration and mood enhancement.

## Features

- **Smart RGB Lighting**: 12-LED NeoPixel ring with customizable colors and dynamic effects
- **Environmental Monitoring**: Real-time temperature and humidity sensing with DHT11 sensor
- **IoT Connectivity**: Full MQTT integration for remote control and data publishing
- **Multi-Network Support**: Automatic WiFi connection with fallback network options
- **Real-time Control**: Instant response to MQTT commands for lighting adjustments
- **JSON Data Streaming**: Structured sensor data transmission every 2 seconds
- **Visual Effects**: Multiple lighting patterns including rainbow cycles and static colors
- **Node-RED Integration**: Pre-configured flows for easy dashboard creation

## Technology Stack

- **Hardware**: ESP8266 (WeMos D1), NeoPixel LED Ring, DHT11 Sensor
- **Programming**: Arduino C++ with optimized libraries
- **Communication**: MQTT Protocol over WiFi
- **Data Format**: JSON for structured data exchange
- **UI Framework**: Node-RED flows for dashboard interface
- **Libraries**: ArduinoJson, PubSubClient, Adafruit_NeoPixel, DFRobot_DHT11

## Quick Start

### Hardware Requirements
- ESP8266 development board (WeMos D1 R2)
- 12-LED NeoPixel ring
- DHT11 temperature/humidity sensor
- Connecting wires and breadboard

### Installation

1. **Clone the repository**
   ```bash
   git clone https://github.com/christoph1j2/ESP8266-MoodLamp.git
   cd ESP8266-MoodLamp
   ```

2. **Install required libraries**
   - Copy libraries from `zdrojove_kody/knihovny/` to your Arduino libraries folder
   - Or install via Arduino Library Manager:
     - ArduinoJson
     - PubSubClient  
     - Adafruit NeoPixel
     - DFRobot_DHT11

3. **Configure WiFi networks**
   Edit `zdrojove_kody/src.ino` and update the WiFi credentials:
   ```cpp
   WifiNetwork wifiNetworks[] = {
     {"YourWiFiName", "YourPassword"},
     {"BackupWiFi", "BackupPassword"}
   };
   ```

4. **Upload to ESP8266**
   - Open `zdrojove_kody/src.ino` in Arduino IDE
   - Select ESP8266 board and appropriate port
   - Upload the code

### Hardware Connections
- **NeoPixel Ring**: Data pin → D1
- **DHT11 Sensor**: Data pin → D0
- **Power**: 3.3V and GND connections

## Usage

### MQTT Control Commands

The device subscribes to several MQTT topics for real-time control:

```bash
# Change lighting mode (0 = rainbow, 1 = static color)
mosquitto_pub -h test.mosquitto.org -t "MoodLamp/LampMode" -m "0"

# Set RGB color components (0-255)
mosquitto_pub -h test.mosquitto.org -t "MoodLamp/Lamp/Red" -m "255"
mosquitto_pub -h test.mosquitto.org -t "MoodLamp/Lamp/Green" -m "128"
mosquitto_pub -h test.mosquitto.org -t "MoodLamp/Lamp/Blue" -m "64"
```

### Data Output

The device publishes sensor data to `MoodLamp` topic every 2 seconds:

```json
{
  "teplota": {
    "hodnota": 23.5,
    "jednotka": "°C"
  },
  "vlhkost": {
    "hodnota": 65.2,
    "jednotka": "%"
  }
}
```

### Node-RED Integration

Import the provided Node-RED flow from `zdrojove_kody/flows.json` to create an instant dashboard with:
- Real-time sensor data visualization
- RGB color controls
- Lighting mode switches
- Temperature and humidity graphs

## System Architecture

```
[WiFi Network] ↔ [ESP8266] ↔ [MQTT Broker]
                     ↓
              [NeoPixel Ring + DHT11]
                     ↓
              [Node-RED Dashboard]
```

The ESP8266 acts as an IoT gateway, collecting environmental data and controlling RGB lighting based on MQTT commands. The system supports multiple WiFi networks for enhanced reliability and publishes structured JSON data for easy integration with IoT platforms.

## API Reference

### MQTT Topics

| Topic | Direction | Description | Payload Example |
|-------|-----------|-------------|-----------------|
| `MoodLamp/LampMode` | Subscribe | Lighting mode control | `0` (rainbow), `1` (static) |
| `MoodLamp/Lamp/Red` | Subscribe | Red color component | `0-255` |
| `MoodLamp/Lamp/Green` | Subscribe | Green color component | `0-255` |
| `MoodLamp/Lamp/Blue` | Subscribe | Blue color component | `0-255` |
| `MoodLamp` | Publish | Sensor data output | JSON object |

## Documentation

- **Circuit Diagrams**: `dokumentace/schema/`
- **Photos**: `dokumentace/fotky/`
- **Design Files**: `dokumentace/design/`
- **Project Requirements**: `zadani.md` (Czech)

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/improvement`)
3. Commit your changes (`git commit -am 'Add new feature'`)
4. Push to the branch (`git push origin feature/improvement`)
5. Create a Pull Request

## License

This project is developed as part of an educational IoT course. Please reference the original project when using or modifying the code.

---

*Built with ❤️ using ESP8266 and modern IoT technologies*
