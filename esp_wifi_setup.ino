#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= RESET =================
#define RESET_PIN D3
#define RESET_HOLD_TIME_MS 2000

// ================= EEPROM =================
#define EEPROM_SIZE 256
#define SSID_ADDR 0
#define PASS_ADDR 64
#define DEVICE_ID_ADDR 128

// ================= WIFI =================
#define MAX_WIFI_CONNECT_ATTEMPTS 40

ESP8266WebServer server(80);
DNSServer dns;

// ================= GLOBAL =================
String ssid, pass;
String deviceID;
String apSSID, apPASS;

// ================= UTIL =================
String randomPrefix() {
  const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  String result = "";
  for (int i = 0; i < 3; i++) result += charset[random(0, 36)];
  return result;
}

String randomAPPass() {
  String password = "";
  for (int i = 0; i < 8; i++) password += String(random(0, 10));
  return password;
}

String generateAPSSID() {
  return "ESP-" + randomPrefix() + String(random(100, 999));
}

// ================= EEPROM =================
void saveString(int addr, String data) {
  for (int i = 0; i < 64; i++) EEPROM.write(addr + i, 0);
  for (int i = 0; i < data.length(); i++) EEPROM.write(addr + i, data[i]);
  EEPROM.commit();
}

String readString(int addr) {
  String result = "";
  for (int i = 0; i < 64; i++) {
    char c = EEPROM.read(addr + i);
    if (c == 0 || c == 255) break;
    result += c;
  }
  return result;
}

// ================= DEVICE ID =================
void loadOrCreateDeviceID() {
  deviceID = readString(DEVICE_ID_ADDR);

  if (deviceID.length() < 5) {
    String chip = String(ESP.getChipId(), HEX);
    chip.toUpperCase();
    deviceID = randomPrefix() + "-" + chip;
    saveString(DEVICE_ID_ADDR, deviceID);
  }
}

// ================= OLED =================
void showLoading() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(10, 28);
  display.println("Initializing Device");

  display.display();
}

void showSetup(IPAddress ip) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.println("SETUP MODE");

  display.setCursor(0, 18);
  display.print("SSID: ");
  display.print(apSSID);

  display.setCursor(0, 32);
  display.print("PASS: ");
  display.print(apPASS);

  display.setCursor(0, 50);
  display.print("IP: ");
  display.print(ip);

  display.display();
}

void showOnline() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.println("ONLINE");

  display.setCursor(0, 25);
  display.print("ID: ");
  display.print(deviceID);

  display.setCursor(0, 45);
  display.print("IP: ");
  display.print(WiFi.localIP());

  display.display();
}

// ================= RESET =================
void factoryReset() {
  for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
  EEPROM.commit();

  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);

  delay(500);
  ESP.restart();
}

void checkReset() {
  if (digitalRead(RESET_PIN) == LOW) {
    delay(RESET_HOLD_TIME_MS);
    if (digitalRead(RESET_PIN) == LOW) {
      factoryReset();
    }
  }
}

// ================= WIFI CONNECT =================
bool connectWiFi() {

  if (ssid.length() == 0 || pass.length() == 0) return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  showLoading();

  int attempts = 0;

  while (attempts < MAX_WIFI_CONNECT_ATTEMPTS) {
    if (WiFi.status() == WL_CONNECTED) return true;
    delay(500);
    attempts++;
  }

  return false;
}

// ================= AP MODE =================
void startAP() {

  randomSeed(micros());

  apSSID = generateAPSSID();
  apPASS = randomAPPass();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID.c_str(), apPASS.c_str());

  IPAddress ip = WiFi.softAPIP();

  dns.start(53, "*", ip);

  server.on("/", []() {
    String html =
      "<h2>ESP Setup</h2>"
      "<p><b>SSID:</b> " + apSSID + "</p>"
      "<p><b>PASS:</b> " + apPASS + "</p>"
      "<p><b>IP:</b> " + WiFi.softAPIP().toString() + "</p>"
      "<a href='/scan'>Configure WiFi</a>";
    server.send(200, "text/html", html);
  });

  server.on("/scan", []() {
    int networkCount = WiFi.scanNetworks();

    String html = "<form method='POST' action='/connect'>";

    for (int i = 0; i < networkCount; i++) {
      html += "<input type='radio' name='ssid' value='" + WiFi.SSID(i) + "'>";
      html += WiFi.SSID(i) + "<br>";
    }

    html += "<input type='password' name='pass' placeholder='Password' required><br>";
    html += "<button type='submit'>Connect</button></form>";

    server.send(200, "text/html", html);
  });

  server.on("/connect", HTTP_POST, []() {

    String newSSID = server.arg("ssid");
    String newPASS = server.arg("pass");

    WiFi.disconnect();
    delay(300);

    WiFi.mode(WIFI_STA);
    WiFi.begin(newSSID.c_str(), newPASS.c_str());

    showLoading();

    int attempts = 0;
    bool connected = false;

    while (attempts < MAX_WIFI_CONNECT_ATTEMPTS) {
      if (WiFi.status() == WL_CONNECTED) {
        connected = true;
        break;
      }
      delay(500);
      attempts++;
    }

    if (connected) {
      saveString(SSID_ADDR, newSSID);
      saveString(PASS_ADDR, newPASS);

      server.send(200, "text/html",
        "<h2>Connected</h2><p>Rebooting...</p>");

      delay(1500);
      ESP.restart();
    } else {
      WiFi.disconnect(true);

      server.send(200, "text/html",
        "<h2 style='color:red;'>Wrong Password</h2><a href='/scan'>Try Again</a>");

      startAP();
    }
  });

  server.begin();

  showSetup(ip);
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  pinMode(RESET_PIN, INPUT_PULLUP);

  Wire.begin(D2, D1);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  showLoading();
  delay(1200);

  ssid = readString(SSID_ADDR);
  pass = readString(PASS_ADDR);

  loadOrCreateDeviceID();

  bool connected = connectWiFi();

  if (!connected) {
    startAP();
  }
}

// ================= LOOP =================
void loop() {
  checkReset();

  dns.processNextRequest();
  server.handleClient();

  if (WiFi.status() == WL_CONNECTED) {
    showOnline();
  }
}
