# esp_wifi_setup

An ESP8266-based WiFi provisioning system with OLED display support, captive portal for wireless configuration, persistent credential storage via EEPROM, and a factory reset function.

---

## Features

- **Captive Portal AP Mode** – Automatically creates a unique access point when no WiFi credentials are saved, allowing users to configure the network via a browser.
- **Persistent EEPROM Storage** – Saves WiFi SSID, password, and a unique device ID to EEPROM so credentials survive power cycles.
- **OLED Display (SSD1306)** – Shows status information including AP credentials, device ID, and local IP address.
- **Unique Device ID** – Generates and stores a human-readable device ID derived from the ESP8266 chip ID on first boot.
- **Factory Reset** – Hold the reset button (D3) for 2 seconds to erase all stored credentials and restart the device.

---

## Hardware Requirements

| Component | Details |
|---|---|
| Board | ESP8266 (NodeMCU / Wemos D1 Mini or compatible) |
| Display | 0.96" SSD1306 OLED (I2C, 128×64) |
| Reset Button | Momentary push button connected to pin **D3** (active LOW) |

### Wiring

| OLED Pin | ESP8266 Pin |
|---|---|
| SDA | D2 |
| SCL | D1 |
| VCC | 3.3V |
| GND | GND |

| Button Pin | ESP8266 Pin |
|---|---|
| One leg | D3 |
| Other leg | GND |

---

## Dependencies / Libraries

Install the following libraries via the Arduino Library Manager:

| Library | Purpose |
|---|---|
| `ESP8266WiFi` | WiFi connectivity (bundled with ESP8266 core) |
| `ESP8266WebServer` | Web server for captive portal (bundled with ESP8266 core) |
| `DNSServer` | DNS redirection for captive portal (bundled with ESP8266 core) |
| `EEPROM` | Credential persistence (bundled with ESP8266 core) |
| `Wire` | I2C communication (bundled with Arduino core) |
| `Adafruit GFX Library` | Graphics primitives for OLED |
| `Adafruit SSD1306` | SSD1306 OLED driver |

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
Read SSID & Password from EEPROM
   │
   ├─ Credentials found ──► Connect to WiFi
   │                              │
   │                         Connected? ──Yes──► Show "ONLINE" on OLED
   │                              │
   │                             No
   │                              │
   └─ No credentials ────────────►▼
                            Start AP Mode (Captive Portal)
                            Show AP SSID / PASS / IP on OLED
```

### AP / Setup Mode

1. The device creates an access point with a randomly generated SSID (e.g. `ESP-XYZ123`) and an 8-digit numeric password.
2. Connect to that AP from any phone or computer.
3. A captive portal page will open automatically (or navigate to `192.168.4.1`).
4. Click **Configure WiFi** to scan for nearby networks.
5. Select your network, enter the password, and submit.
6. The device connects to your network, saves the credentials, and reboots.

### Factory Reset

- Hold the button on pin **D3** for **2 seconds**.
- All EEPROM data (SSID, password, device ID) is erased.
- The device restarts and enters AP mode again.

---

## EEPROM Memory Map

| Address | Length | Content |
|---|---|---|
| 0 | 64 bytes | WiFi SSID |
| 64 | 64 bytes | WiFi Password |
| 128 | 64 bytes | Device ID |

---

## Web Endpoints

| Endpoint | Method | Description |
|---|---|---|
| `/` | GET | Home page – shows AP SSID, password, and IP |
| `/scan` | GET | Scans for available WiFi networks and shows a selection form |
| `/connect` | POST | Receives selected SSID and password, attempts connection |

---

## Source Code

The full source code is in [`esp_wifi_setup.ino`](./esp_wifi_setup.ino).

---

## License

This project is licensed under the terms found in the [LICENSE](./LICENSE) file.
