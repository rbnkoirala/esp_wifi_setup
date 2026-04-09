#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= SERVER =================
ESP8266WebServer server(80);
DNSServer dns;

// ================= EEPROM =================
#define EEPROM_SIZE 512

#define SSID_ADDR 0
#define PASS_ADDR 64
#define MQTT_HOST_ADDR 128
#define MQTT_PORT_ADDR 192
#define MQTT_USER_ADDR 224
#define MQTT_PASS_ADDR 288

// ================= WIFI =================
String ssid = "";
String pass = "";

// ================= MQTT =================
WiFiClient espClient;
PubSubClient mqtt(espClient);

String mqttHost = "";
int mqttPort = 1883;
String mqttUser = "";
String mqttPass = "";
String mqttClientID;

bool mqttConnected = false;

// ================= OLED =================
void oled(String l1, String l2, String l3, String l4) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);  display.println(l1);
  display.setCursor(0, 16); display.println(l2);
  display.setCursor(0, 32); display.println(l3);
  display.setCursor(0, 48); display.println(l4);

  display.display();
}

// ================= EEPROM HELPERS =================
void saveString(int addr, String data) {
  for (int i = 0; i < 64; i++) EEPROM.write(addr + i, 0);
  for (int i = 0; i < data.length(); i++) EEPROM.write(addr + i, data[i]);
  EEPROM.commit();
}

String readString(int addr) {
  String s = "";
  for (int i = 0; i < 64; i++) {
    char c = EEPROM.read(addr + i);
    if (c == 0) break;
    s += c;
  }
  return s;
}

// ================= MQTT CONNECT =================
void connectMQTT() {

  if (mqttHost == "") return;

  mqtt.setServer(mqttHost.c_str(), mqttPort);

  mqttClientID = "ESP-" + String(ESP.getChipId());

  if (mqtt.connect(mqttClientID.c_str(), mqttUser.c_str(), mqttPass.c_str())) {
    mqttConnected = true;
  } else {
    mqttConnected = false;
  }
}

// ================= WIFI CONNECT =================
bool connectWiFi() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  oled("CONNECTING WIFI", ssid, "", "");

  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 40) {
    delay(500);
    t++;
  }

  return WiFi.status() == WL_CONNECTED;
}

// ================= SAVE MQTT =================
void saveMQTT() {

  mqttHost = server.arg("host");
  mqttPort = server.arg("port").toInt();
  mqttUser = server.arg("user");
  mqttPass = server.arg("pass");

  saveString(MQTT_HOST_ADDR, mqttHost);
  saveString(MQTT_PORT_ADDR, String(mqttPort));
  saveString(MQTT_USER_ADDR, mqttUser);
  saveString(MQTT_PASS_ADDR, mqttPass);

  connectMQTT();

  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// ================= RESET =================
void handleReset() {

  for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
  EEPROM.commit();

  server.send(200, "text/html", "RESET DONE. REBOOTING...");
  delay(1000);
  ESP.restart();
}

// ================= MQTT UI =================
void handleRoot() {

  String statusColor = mqttConnected ? "#00c853" : "#d50000";
  String statusText = mqttConnected ? "CONNECTED" : "DISCONNECTED";

  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>MQTT PANEL</title>

<style>
body {
  font-family: Arial;
  background: #0f172a;
  color: white;
  text-align: center;
  padding: 20px;
}

.card {
  background: #1e293b;
  padding: 20px;
  border-radius: 15px;
  max-width: 360px;
  margin: auto;
}

input {
  width: 90%;
  padding: 10px;
  margin: 6px;
  border-radius: 8px;
  border: none;
}

button {
  width: 95%;
  padding: 10px;
  margin-top: 10px;
  border: none;
  border-radius: 8px;
  font-weight: bold;
}

.save { background: #3b82f6; color: white; }
.reset { background: #ef4444; color: white; }

.status {
  padding: 10px;
  border-radius: 8px;
  margin-bottom: 10px;
  font-weight: bold;
}
</style>
</head>

<body>

<div class="card">
<h2>MQTT CONFIG</h2>

<div class="status" style="background:)rawliteral" + statusColor + R"rawliteral(">
)rawliteral" + statusText + R"rawliteral(
</div>
)rawliteral";

  if (!mqttConnected) {
    html += R"rawliteral(
<form method='POST' action='/mqtt'>
<input name='host' placeholder='MQTT Host'>
<input name='port' placeholder='Port'>
<input name='user' placeholder='Username'>
<input name='pass' type='password' placeholder='Password'>
<button class='save'>CONNECT</button>
</form>
)rawliteral";
  }

  html += R"rawliteral(
<form method='POST' action='/reset'>
<button class='reset'>RESET DEVICE</button>
</form>

</div>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// ================= LOAD MQTT =================
void loadMQTT() {

  mqttHost = readString(MQTT_HOST_ADDR);
  mqttPort = readString(MQTT_PORT_ADDR).toInt();
  mqttUser = readString(MQTT_USER_ADDR);
  mqttPass = readString(MQTT_PASS_ADDR);
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  Wire.begin(D2, D1);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true);
  }

  oled("BOOTING", "ESP DEVICE", "", "");

  ssid = readString(SSID_ADDR);
  pass = readString(PASS_ADDR);

  loadMQTT();

  if (ssid.length() > 0 && connectWiFi()) {

    server.on("/", handleRoot);
    server.on("/mqtt", HTTP_POST, saveMQTT);
    server.on("/reset", HTTP_POST, handleReset);

    server.begin();

    connectMQTT();

    oled("ONLINE",
         WiFi.localIP().toString(),
         mqttConnected ? "MQTT ONLINE" : "MQTT OFFLINE",
         "");

  } else {

    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP-SETUP", "12345678");

    oled("SETUP MODE", "ESP-SETUP", WiFi.softAPIP().toString(), "");

    server.on("/", handleRoot);
    server.on("/mqtt", HTTP_POST, saveMQTT);
    server.on("/reset", HTTP_POST, handleReset);

    server.begin();
  }
}

// ================= LOOP =================
void loop() {

  server.handleClient();

  if (WiFi.status() == WL_CONNECTED) {

    if (!mqtt.connected()) {
      connectMQTT();
    }

    mqtt.loop();
    mqttConnected = mqtt.connected();

    oled(
      "ONLINE",
      WiFi.localIP().toString(),
      mqttConnected ? "MQTT ONLINE" : "MQTT OFFLINE",
      "RUNNING"
    );
  }
}
