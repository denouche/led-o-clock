# Led'o'clock Firmware

This folder contains the ESP8266 source code built with **PlatformIO**. It manages the WiFi portal, NTP synchronization, and the LED ring animation logic.

## Key Features

* **Built-in Web Server**: Serves a responsive UI stored entirely in Flash memory (PROGMEM).
* **RESTful API**: Allows full control of the device through simple HTTP requests.
* **mDNS Support**: Discovers the device on the local network as `ledoclock.local`.
* **7-Day Scheduler**: Robust time-keeping logic that handles weekly routines and internet outages.
* **NTP Sync**: Automatically fetches time at boot.

## REST API Endpoints

The firmware exposes the following endpoints for automation:

| Endpoint | Method | Description |
| :--- | :--- | :--- |
| `/status` | `GET` | Returns a JSON object with current time, color, and all schedules. |
| `/set_brightness?value=X` | `GET` | Sets the ring brightness (0 to 100). |
| `/set_color?value=NAME` | `GET` | Manually forces a color (e.g., `red`, `green`, `off`). |
| `/colors` | `POST` | Updates the hex definitions of custom colors (JSON body). |
| `/schedule` | `POST` | Updates the full weekly schedule (JSON body). |
| `/reset_wifi` | `GET` | Wipes WiFi credentials and restarts in Access Point mode. |
| `/reset` | `GET` | Wipes all settings (WiFi + saved configuration) and restarts in Access Point mode. |

## Hardware Mapping

* **GPIO 2**: WS2812B Data Line (connected to the LED ring).
* **GPIO 0**: Multi-purpose button (Pull to GND at boot for **Flash Mode** / Long press for **Factory Reset**).

## Installation

### Prerequisites

* **PlatformIO IDE** (VS Code extension recommended).
* **ESP-01S** module + USB-to-Serial adapter.

### Setup & Upload

1. Open this folder in VS Code/PlatformIO.
2. The `platformio.ini` is already configured for a 1MB flash module with 64KB dedicated to **LittleFS**.
3. Connect your ESP-01S in **Flash Mode**.
4. Run the **Upload** task.

### Debugging

The Serial Monitor speed is set to `74880` baud to capture the ESP8266 bootrom messages along with application logs.

### Development Notes

The UI is built using vanilla JavaScript and CSS, embedded in C++ header files. It uses asynchronous `fetch` calls to interact with the API endpoints listed above, ensuring a smooth user experience even on the limited ESP-01S hardware.
