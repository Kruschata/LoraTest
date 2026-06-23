#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WebServer.h>
#include "BluetoothSerial.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "index.h"

// Change this before flashing each LilyGO: 1, 2, 3, ...
static const uint8_t DEVICE_ID = 4;
static const char *BT_NAME = "LoraChat-4";
static const char *WIFI_AP_SSID = "LoraChat-4";
static const char *WIFI_AP_PASSWORD = "12345678";

// LILYGO T-Beam AXP2101 with SX1276/SX1278.
static const long LORA_FREQUENCY = 868E6;

static const int PIN_LORA_SCK = 5;
static const int PIN_LORA_MISO = 19;
static const int PIN_LORA_MOSI = 27;
static const int PIN_LORA_SS = 18;
static const int PIN_LORA_RST = 23;
static const int PIN_LORA_DIO0 = 26;

static const int PIN_DISPLAY_SCL = 22;
static const int PIN_DISPLAY_SDA = 21;
static const int SCREEN_WIDTH = 128;
static const int SCREEN_HEIGHT = 64;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
String lastDisplayMessage = "-";
int lastRSSI = 0;

BluetoothSerial SerialBT;
WebServer webServer(80);

uint32_t messageCounter = 0;
String usbLine;
String btLine;

static const uint8_t MESSAGE_LOG_SIZE = 20;
String messageLog[MESSAGE_LOG_SIZE];
uint8_t messageLogStart = 0;
uint8_t messageLogCount = 0;

// ============================================================
// DISPLAY - Funktionen für das OLED-Display
// ============================================================

void updateDisplay() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("Name:");
  display.println("LoraChat-" + String(DEVICE_ID));

  display.setCursor(0, 16);
  display.print("RSSI: " );
  display.print(lastRSSI);
  display.println("dbm");

  display.setCursor(0, 32);
  display.println("Msg:");

  String msg = lastDisplayMessage;
  if (msg.length() > 40) {
    msg = msg.substring(0, 40);
  }

  display.setCursor(0, 44);
  display.println(msg);

  display.display();
}

// ============================================================
// UTILITY - Hilfsfunktionen für Escaping und String-Verarbeitung
// ============================================================

String escapeField(const String &value) {
  String out;
  out.reserve(value.length());

  for (size_t i = 0; i < value.length(); i++) {
    char c = value.charAt(i);
    if (c == '\\' || c == '|') {
      out += '\\';
    }
    if (c != '\r' && c != '\n') {
      out += c;
    }
  }

  return out;
}

String unescapeField(const String &value) {
  String out;
  out.reserve(value.length());
  bool escaped = false;

  for (size_t i = 0; i < value.length(); i++) {
    char c = value.charAt(i);
    if (escaped) {
      out += c;
      escaped = false;
    } else if (c == '\\') {
      escaped = true;
    } else {
      out += c;
    }
  }

  return out;
}

int findUnescapedPipe(const String &value, int startAt) {
  bool escaped = false;

  for (int i = startAt; i < value.length(); i++) {
    char c = value.charAt(i);
    if (escaped) {
      escaped = false;
    } else if (c == '\\') {
      escaped = true;
    } else if (c == '|') {
      return i;
    }
  }

  return -1;
}

// ============================================================
// MESSAGE LOG - Verwaltung des Message-Buffers und Ausgabe
// ============================================================

//Ringpuffer für die letzten 20 Nachrichten
void addMessageLog(const String &line) {
  uint8_t index;
  if (messageLogCount < MESSAGE_LOG_SIZE) {
    index = (messageLogStart + messageLogCount) % MESSAGE_LOG_SIZE;
    messageLogCount++;
  } else {
    index = messageLogStart;
    messageLogStart = (messageLogStart + 1) % MESSAGE_LOG_SIZE;
  }

  messageLog[index] = line;
}

// Ausgabe einer Zeile auf USB und Bluetooth
void emitLine(const String &line) {
  Serial.println(line);
  if (SerialBT.hasClient()) {
    SerialBT.println(line);
  }
}

// Ausgabe einer Zeile auf USB, Bluetooth und in den Message-Log
void emitChatLine(const String &line) {
  addMessageLog(line);
  emitLine(line);
}

// ============================================================
// LORA - LoRa Kommunikation und Chat-Funktionen
// ============================================================

void sendChatMessage(String text) {
  text.trim();
  if (text.length() == 0) {
    return;
  }

  if (text.length() > 180) {
    text = text.substring(0, 180);
  }

  messageCounter++;
  String escapedText = escapeField(text);
  String payload = "LCHAT|1|" + String(DEVICE_ID) + "|" + String(messageCounter) + "|" + escapedText;

  LoRa.idle();
  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();
  LoRa.receive();

  emitChatLine("TX|" + String(DEVICE_ID) + "|" + String(messageCounter) + "|" + escapedText);

  lastDisplayMessage = text;
  updateDisplay();
}

void handleCommand(String line) {
  line.trim();
  if (line.length() == 0) {
    return;
  }

  if (line.startsWith("CHAT|")) {
    sendChatMessage(line.substring(5));
  } else {
    sendChatMessage(line);
  }
}

void readCommandStream(Stream &stream, String &buffer) {
  while (stream.available()) {
    char c = (char)stream.read();
    if (c == '\n') {
      handleCommand(buffer);
      buffer = "";
    } else if (c != '\r') {
      if (buffer.length() < 220) {
        buffer += c;
      }
    }
  }
}

void handleIncomingLoRa() {
  int packetSize = LoRa.parsePacket();
  if (packetSize <= 0) {
    return;
  }

  String payload;
  while (LoRa.available()) {
    payload += (char)LoRa.read();
  }
  payload.trim();

  int p1 = payload.indexOf('|');
  int p2 = payload.indexOf('|', p1 + 1);
  int p3 = payload.indexOf('|', p2 + 1);
  int p4 = findUnescapedPipe(payload, p3 + 1);

  if (!payload.startsWith("LCHAT|") || p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0) {
    emitLine("RAW|" + String(LoRa.packetRssi()) + "|" + String(LoRa.packetSnr()) + "|" + escapeField(payload));
    return;
  }

  String from = payload.substring(p2 + 1, p3);
  String counter = payload.substring(p3 + 1, p4);
  String text = unescapeField(payload.substring(p4 + 1));
  lastRSSI = LoRa.packetRssi();
  lastDisplayMessage = text;
  updateDisplay();

  if (from.toInt() == DEVICE_ID) {
    return;
  }

  emitChatLine(
    "RX|" + from + "|" + counter + "|" + String(LoRa.packetRssi()) + "|" +
    String(LoRa.packetSnr()) + "|" + escapeField(text)
  );
}

// ============================================================
// WEBSERVER - HTTP API für die Web-Schnittstelle
// ============================================================

void handleWebRoot() {
  webServer.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleWebMessages() {
  String response;
  for (uint8_t i = 0; i < messageLogCount; i++) {
    uint8_t index = (messageLogStart + i) % MESSAGE_LOG_SIZE;
    response += messageLog[index];
    response += '\n';
  }
  webServer.send(200, "text/plain; charset=utf-8", response);
}

void handleWebSend() {
  String text = webServer.arg("plain");
  if (text.length() == 0 && webServer.hasArg("text")) {
    text = webServer.arg("text");
  }

  text.trim();
  if (text.length() == 0) {
    webServer.send(400, "text/plain; charset=utf-8", "empty message");
    return;
  }

  sendChatMessage(text);
  webServer.send(200, "text/plain; charset=utf-8", "ok");
}

void setupWebServer() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);

  webServer.on("/", HTTP_GET, handleWebRoot);
  webServer.on("/api/messages", HTTP_GET, handleWebMessages);
  webServer.on("/api/send", HTTP_POST, handleWebSend);
  webServer.onNotFound([]() {
    webServer.send(404, "text/plain; charset=utf-8", "not found");
  });
  webServer.begin();

  emitLine("STATUS|OK|WIFI|" + String(WIFI_AP_SSID) + "|IP|192.168.4.1");
}

// ============================================================
// SETUP & LOOP - Initialisierung und Hauptschleife
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(500);
  Wire.begin(PIN_DISPLAY_SDA, PIN_DISPLAY_SCL);

if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
  updateDisplay();
}

  SerialBT.begin(BT_NAME);

  SPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_SS);
  LoRa.setPins(PIN_LORA_SS, PIN_LORA_RST, PIN_LORA_DIO0);

  if (!LoRa.begin(LORA_FREQUENCY)) {
    emitLine("STATUS|ERROR|LoRa init failed");
    while (true) {
      delay(1000);
    }
  }

  LoRa.setSyncWord(0x34);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setTxPower(17);
  LoRa.enableCrc();
  LoRa.receive();

  setupWebServer();

  emitLine("STATUS|OK|DEVICE|" + String(DEVICE_ID) + "|BT|" + String(BT_NAME));
}

void loop() {
  webServer.handleClient();
  readCommandStream(Serial, usbLine);
  readCommandStream(SerialBT, btLine);
  handleIncomingLoRa();
}
