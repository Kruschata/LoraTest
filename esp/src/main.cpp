#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPSPlus.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// Change this before flashing each LilyGO: 1, 2, 3, ...
static const uint8_t DEVICE_ID =1;
// Give every node a unique name (and DEVICE_ID) before flashing it.
static const char *BT_NAME = "LoRaChat-1";

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

uint32_t messageCounter = 0;
String usbLine;
String btLine;

static const uint8_t MESSAGE_LOG_SIZE = 20;
String messageLog[MESSAGE_LOG_SIZE];
uint8_t messageLogStart = 0;
uint8_t messageLogCount = 0;

TinyGPSPlus gps;
HardwareSerial GPSSerial(1);

// T-Beam GPS defaults (NEO-6M). Adjust if your hardware revision differs.
static const int PIN_GPS_RX = 34;
static const int PIN_GPS_TX = 12;
// Many T-Beam revisions require an explicit GPS power enable on GPIO4.
static const int PIN_GPS_POWER = 4;
static const uint32_t GPS_BAUD_RATE = 9600;
static const uint32_t LOC_SEND_INTERVAL_MS = 15000;
static const uint32_t GPS_STATUS_INTERVAL_MS = 30000;

uint32_t locationCounter = 0;
unsigned long lastLocSendMs = 0;
unsigned long lastGpsStatusMs = 0;
uint32_t gpsCharsAtLastStatus = 0;
bool gpsPowerEnabled = false;

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
}

bool initDisplay() {
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    return true;
  }

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    return true;
  }

  return false;
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

bool axpReadRegister(uint8_t reg, uint8_t &value) {
  Wire.beginTransmission(0x34);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom((uint8_t)0x34, (uint8_t)1) != 1) {
    return false;
  }

  value = Wire.read();
  return true;
}

bool axpWriteRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(0x34);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool tryEnableGpsPowerViaAxp192() {
  uint8_t powerReg = 0;
  if (!axpReadRegister(0x12, powerReg)) {
    return false;
  }

  // AXP192 register 0x12: enable LDO2/LDO3 rails commonly used for GPS power.
  const uint8_t newValue = powerReg | 0x0C;
  if (!axpWriteRegister(0x12, newValue)) {
    return false;
  }

  uint8_t verify = 0;
  if (!axpReadRegister(0x12, verify)) {
    return false;
  }

  return (verify & 0x0C) == 0x0C;
}

void enableGpsPower() {
  pinMode(PIN_GPS_POWER, OUTPUT);
  digitalWrite(PIN_GPS_POWER, HIGH);

  gpsPowerEnabled = true;
  emitLine("STATUS|INFO|GPS power enabled via GPIO only");
}

String gpsTimestampIsoUtc() {
  if (!gps.date.isValid() || !gps.time.isValid()) {
    return "";
  }

  char buffer[25];
  snprintf(
    buffer,
    sizeof(buffer),
    "%04d-%02d-%02dT%02d:%02d:%02dZ",
    gps.date.year(),
    gps.date.month(),
    gps.date.day(),
    gps.time.hour(),
    gps.time.minute(),
    gps.time.second()
  );

  return String(buffer);
}

void emitLocLine(
  const String &nodeId,
  double lat,
  double lon,
  double accuracyMeters,
  const String &timestampUtc
) {
  String line = "LOC|" + nodeId + "|" + String(lat, 6) + "|" + String(lon, 6) + "|" + String(accuracyMeters, 1);
  if (timestampUtc.length() > 0) {
    line += "|" + timestampUtc;
  }
  emitLine(line);
}

void sendLocationBroadcast() {
  if (!gps.location.isValid()) {
    return;
  }

  locationCounter++;
  const double lat = gps.location.lat();
  const double lon = gps.location.lng();
  const double hdopMeters = gps.hdop.isValid() ? gps.hdop.hdop() * 5.0 : 999.0;
  const String timestampUtc = gpsTimestampIsoUtc();

  String payload =
    "LLOC|1|" + String(DEVICE_ID) + "|" + String(locationCounter) + "|" +
    String(lat, 6) + "|" + String(lon, 6) + "|" + String(hdopMeters, 1);

  if (timestampUtc.length() > 0) {
    payload += "|" + timestampUtc;
  }

  LoRa.idle();
  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();
  LoRa.receive();

  // Also emit local node location immediately to USB/Bluetooth app clients.
  emitLocLine(String(DEVICE_ID), lat, lon, hdopMeters, timestampUtc);
}

void emitGpsStatus() {
  const uint32_t charsNow = gps.charsProcessed();
  const uint32_t charsDelta = charsNow - gpsCharsAtLastStatus;
  gpsCharsAtLastStatus = charsNow;

  if (charsNow < 10) {
    emitLine("STATUS|GPS|NO_DATA|check_power_or_pins");
    return;
  }

  if (!gps.location.isValid()) {
    emitLine(
      "STATUS|GPS|NO_FIX|chars_delta|" + String(charsDelta) +
      "|sats|" + (gps.satellites.isValid() ? String(gps.satellites.value()) : String("?")) +
      "|hdop|" + (gps.hdop.isValid() ? String(gps.hdop.hdop(), 1) : String("?"))
    );
    return;
  }

  emitLine(
    "STATUS|GPS|FIX|lat|" + String(gps.location.lat(), 6) +
    "|lon|" + String(gps.location.lng(), 6) +
    "|sats|" + (gps.satellites.isValid() ? String(gps.satellites.value()) : String("?")) +
    "|hdop|" + (gps.hdop.isValid() ? String(gps.hdop.hdop(), 1) : String("?"))
  );
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

  if (line == "GPS|STATUS") {
    emitGpsStatus();
    return;
  }

  if (line == "GPS|POWERON") {
    enableGpsPower();
    emitLine("STATUS|GPS|POWER|ON");
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

  if (payload.startsWith("LLOC|")) {
    int p1 = payload.indexOf('|');
    int p2 = payload.indexOf('|', p1 + 1);
    int p3 = payload.indexOf('|', p2 + 1);
    int p4 = payload.indexOf('|', p3 + 1);
    int p5 = payload.indexOf('|', p4 + 1);
    int p6 = payload.indexOf('|', p5 + 1);

    if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0 || p5 < 0 || p6 < 0) {
      emitLine("RAW|" + String(LoRa.packetRssi()) + "|" + String(LoRa.packetSnr()) + "|" + escapeField(payload));
      return;
    }

    int p7 = payload.indexOf('|', p6 + 1);

    String from = payload.substring(p2 + 1, p3);
    if (from.toInt() == DEVICE_ID) {
      return;
    }

    String latField = payload.substring(p4 + 1, p5);
    String lonField = payload.substring(p5 + 1, p6);
    String accField;
    String tsField;

    if (p7 < 0) {
      accField = payload.substring(p6 + 1);
    } else {
      accField = payload.substring(p6 + 1, p7);
      tsField = payload.substring(p7 + 1);
    }

    emitLocLine(from, latField.toDouble(), lonField.toDouble(), accField.toDouble(), tsField);
    return;
  }

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
// SETUP & LOOP - Initialisierung und Hauptschleife
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(500);

  // Init I2C early: needed for OLED and possible AXP power management.
  Wire.begin(PIN_DISPLAY_SDA, PIN_DISPLAY_SCL);

  enableGpsPower();
  delay(100);

  GPSSerial.begin(GPS_BAUD_RATE, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
  emitLine("STATUS|INFO|GPS serial initialized");

  // Display
  if (initDisplay()) {
    updateDisplay();
  } else {
    emitLine("STATUS|ERROR|OLED init failed");
  }

  // Bluetooth Classic / SPP. A fixed PIN makes pairing with Android reliable.
  if (!SerialBT.begin(BT_NAME)) {
    emitLine("STATUS|ERROR|Bluetooth init failed");
  } else {
    SerialBT.setPin("1234");
    SerialBT.onAuthComplete([](boolean success) {
      emitLine(success
        ? "STATUS|OK|Bluetooth pairing successful"
        : "STATUS|ERROR|Bluetooth pairing failed");
    });
    emitLine("STATUS|OK|Bluetooth|" + String(BT_NAME) + "|PIN|1234");
  }

  // LoRa
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

  emitLine("STATUS|OK|Device " + String(DEVICE_ID) + " ready");
}

void loop() {
  while (GPSSerial.available()) {
    gps.encode((char)GPSSerial.read());
  }

  if (millis() - lastGpsStatusMs >= GPS_STATUS_INTERVAL_MS) {
    lastGpsStatusMs = millis();
    emitGpsStatus();
  }

  if (millis() - lastLocSendMs >= LOC_SEND_INTERVAL_MS) {
    lastLocSendMs = millis();
    sendLocationBroadcast();
  }

  readCommandStream(Serial, usbLine);
  readCommandStream(SerialBT, btLine);
  handleIncomingLoRa();
}
