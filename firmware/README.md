# Led'o'clock Firmware

This folder contains the ESP32-C3 source code built with **PlatformIO**. It manages the WiFi portal, NTP synchronization, and the LED ring animation logic.

## Key Features

* **Built-in Web Server**: Serves a responsive UI stored entirely in Flash memory (embedded via `board_build.embed_txtfiles`).
* **RESTful API**: Allows full control of the device through simple HTTP requests.
* **mDNS Support**: Discovers the device on the local network as `ledoclock-<mac>.local` (unique per device).
* **7-Day Scheduler**: Robust time-keeping logic that handles weekly routines and internet outages.
* **NTP Sync**: Automatically fetches time at boot and applies the configured POSIX timezone.
* **OTA Updates**: Firmware can be updated over-the-air via the `GET /firmware_update` endpoint.
* **WiFi Mesh Roaming**: Automatically connects to the strongest access point in a mesh network.

## REST API Endpoints

The firmware exposes the following endpoints for automation:

| Endpoint | Method | Description |
| :--- | :--- | :--- |
| `/status` | `GET` | Returns a JSON object with version, time, brightness, colors, timezone, and schedules. |
| `/set_brightness?value=X` | `GET` | Sets the ring brightness (0 to 100). |
| `/set_color?value=NAME` | `GET` | Manually forces a color (e.g., `red`, `green`, `off`). |
| `/set_timezone?value=TZ` | `GET` | Updates the device timezone (POSIX string, e.g., `CET-1CEST,M3.5.0,M10.5.0/3`). |
| `/colors` | `POST` | Updates the hex definitions of custom colors (JSON body). |
| `/schedule` | `POST` | Updates the full weekly schedule (JSON body). |
| `/firmware_update` | `GET` | Triggers an OTA firmware update check. |
| `/reset_wifi` | `GET` | Wipes WiFi credentials and restarts in Access Point mode. |
| `/reset` | `GET` | Wipes all settings (WiFi + saved configuration) and restarts in Access Point mode. |
| `/ping` | `GET` | Returns `{"status":"pong"}` — useful for connectivity checks. |

## Hardware Mapping

* **GPIO 3**: WS2812B Data Line (connected to the LED ring).
* **GPIO 9**: Multi-purpose button (built-in BOOT button — hold at boot for **Flash Mode** / Long press (5 s) during runtime for **Factory Reset**).

## LED Status Signals

The ring provides visual feedback for system events:

| Color | Meaning |
| :--- | :--- |
| 🔵 Light Blue | Device is booting and attempting to connect to WiFi. |
| 🟡 Yellow | WiFi Configuration mode — Access Point is active. |
| 🟢 Green (×5 blinks) | WiFi connection successful. |
| 🟣 Purple | Firmware OTA update in progress. |
| 🔴 Red | Factory Reset in progress. |

## Installation

### Prerequisites

* **PlatformIO IDE** (VS Code extension recommended).
* **ESP32-C3** module with native USB (no USB-to-Serial adapter required).

### Setup & Upload

1. Open this folder in VS Code/PlatformIO.
2. The `platformio.ini` is already configured for a 4MB flash module using the **`min_spiffs`** partition scheme (LittleFS).
3. Connect your ESP32-C3 via USB.
4. Run the **Upload** task.

### Debugging

The Serial Monitor speed is set to `115200` baud.

### Development Notes

The UI is built using vanilla JavaScript and CSS, embedded in C++ as binary data via the `board_build.embed_txtfiles` PlatformIO directive. It uses asynchronous `fetch` calls to interact with the API endpoints listed above, ensuring a smooth user experience.
