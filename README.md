# esp_wifi_setup

An ESP8266 firmware that connects to a saved WiFi network, then runs a web panel to configure and connect to an MQTT broker. An SSD1306 OLED display shows live status. All credentials are persisted to EEPROM.

---

## Features

- **WiFi Connection** – Reads SSID and password from EEPROM on boot and connects automatically.
- **AP / Setup Mode** – If no WiFi credentials are stored (or the connection fails), starts a fixed access point (`ESP-SETUP` / `12345678`) so the device is still reachable.
- **MQTT Client** – Connects to a configurable MQTT broker; automatically reconnects in the main loop.
- **MQTT Config Panel** – A browser-based UI (served from the device) lets you enter broker host, port, username, and password.
- **Persistent EEPROM Storage** – WiFi and MQTT credentials survive power cycles (512-byte EEPROM).
- **OLED Display (SSD1306)** – Shows boot, connecting, online/offline, and MQTT status on a 128×64 display.
- **Web Reset** – A button in the web panel wipes all EEPROM data and reboots the device.

---

## Hardware Requirements

| Component | Details |
|---|---|
| Board | ESP8266 (NodeMCU / Wemos D1 Mini or compatible) |
| Display | 0.96" SSD1306 OLED (I2C, 128×64) |

### Wiring

| OLED Pin | ESP8266 Pin |
|---|---|
| SDA | D2 |
| SCL | D1 |
| VCC | 3.3V |
| GND | GND |

---

## Dependencies / Libraries

Install the following libraries via the Arduino Library Manager:

| Library | Purpose |
|---|---|
| `ESP8266WiFi` | WiFi connectivity (bundled with ESP8266 core) |
| `ESP8266WebServer` | Web server for the config panel (bundled with ESP8266 core) |
| `DNSServer` | DNS server support (bundled with ESP8266 core) |
| `EEPROM` | Credential persistence (bundled with ESP8266 core) |
| `Wire` | I2C communication (bundled with Arduino core) |
| `Adafruit GFX Library` | Graphics primitives for OLED |
| `Adafruit SSD1306` | SSD1306 OLED driver |
| `PubSubClient` | MQTT client |

---

## Installation

1. Install the [Arduino IDE](https://www.arduino.cc/en/software) (1.8.x or 2.x).
2. Add the ESP8266 board package via **File → Preferences → Additional Board Manager URLs**:
   ```
   https://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Install the ESP8266 board from **Tools → Board → Board Manager**.
4. Install the required libraries listed above via **Tools → Manage Libraries**.
5. Open `esp_wifi_setup.ino` in the Arduino IDE.
6. Select your board under **Tools → Board** (e.g., `NodeMCU 1.0`).
7. Select the correct **Port**.
8. Click **Upload**.

---

## How It Works

### Boot Sequence

```
Power On
   │
   ▼
Read WiFi SSID & Password from EEPROM
   │
   ├─ Credentials found ──► Connect to WiFi (20-second timeout)
   │                              │
   │                         Connected? ──Yes──► Load MQTT creds → Connect MQTT
   │                              │                    │
   │                              │              Show "ONLINE" + IP + MQTT status
   │                             No
   │                              │
   └─ No credentials ────────────►▼
                            Start AP Mode
                            SSID: ESP-SETUP  Password: 12345678
                            Show "SETUP MODE" on OLED
```

### AP / Setup Mode

When no WiFi credentials are stored or the connection fails, the device starts an access point:

- **SSID:** `ESP-SETUP`
- **Password:** `12345678`
- **IP:** `192.168.4.1`

Connect to the AP from any phone or computer and open `192.168.4.1` to reach the MQTT config panel.

> WiFi credentials (SSID / password) must be written directly to EEPROM addresses 0 and 64 before the device can join a network.

### MQTT Configuration

1. Open the web panel at the device IP (connected mode) or `192.168.4.1` (AP mode).
2. Fill in **MQTT Host**, **Port**, **Username**, and **Password**.
3. Click **CONNECT** – the device saves the credentials and attempts to connect immediately.
4. The panel shows **CONNECTED** (green) or **DISCONNECTED** (red).

### Main Loop

- Handles incoming HTTP requests.
- If WiFi is connected, checks the MQTT connection and reconnects if needed.
- Updates the OLED every loop iteration with current online/MQTT status.

### Factory Reset (Web)

- Open the web panel and click **RESET DEVICE**.
- All 512 bytes of EEPROM are erased.
- The device reboots and enters AP mode.

---

## EEPROM Memory Map

| Address | Length | Content |
|---|---|---|
| 0 | 64 bytes | WiFi SSID |
| 64 | 64 bytes | WiFi Password |
| 128 | 64 bytes | MQTT Host |
| 192 | 64 bytes | MQTT Port (stored as string) |
| 224 | 64 bytes | MQTT Username |
| 288 | 64 bytes | MQTT Password |

Total EEPROM size: **512 bytes**.

---

## Web Endpoints

| Endpoint | Method | Description |
|---|---|---|
| `/` | GET | MQTT config panel – shows connection status and config form |
| `/mqtt` | POST | Save MQTT credentials and attempt connection |
| `/reset` | POST | Erase all EEPROM data and reboot |

---

## MQTT Client ID

The client ID is derived from the ESP8266 chip ID: `ESP-<chipId>` (e.g. `ESP-1234567`).

---

## Source Code

The full source code is in [`esp_wifi_setup.ino`](./esp_wifi_setup.ino).

---

## License

This project is licensed under the terms found in the [LICENSE](./LICENSE) file.
