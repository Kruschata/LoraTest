#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPSPlus.h>
#include "BluetoothSerial.h"
#include "XPowersLib.h"

#define DISABLE_BEACONS 1
#define hal_init lmic_hal_init
#define hal_init_ex lmic_hal_init_ex
#include <lmic.h>
#include <hal/hal.h>

#pragma region CONFIGURATION & GLOBAL STATE

// Bluetooth configuration
static const uint8_t DEVICE_ID = 1;
static const char *BT_NAME_BASE = "BlackoutBuddy";
static String BT_NAME = String(BT_NAME_BASE) + "-" + String(DEVICE_ID);

enum class DeviceMode {
  BLACKOUT,
  TTN
};

static DeviceMode currentMode = DeviceMode::BLACKOUT;
static bool ttnJoined = false;
static bool ttnReady = false;
static String ttnPendingText;

// LoRa configuration
static const long LORA_FREQUENCY = 868E6;
static const int PIN_LORA_SCK = 5;
static const int PIN_LORA_MISO = 19;
static const int PIN_LORA_MOSI = 27;
static const int PIN_LORA_SS = 18;
static const int PIN_LORA_RST = 23;
static const int PIN_LORA_DIO0 = 26;

// Display configuration
static const int PIN_DISPLAY_SCL = 22;
static const int PIN_DISPLAY_SDA = 21;
static const int SCREEN_WIDTH = 128;
static const int SCREEN_HEIGHT = 64;


// GPS configuration
static const int PIN_GPS_RX = 34;
static const int PIN_GPS_TX = 12;
static const int PIN_GPS_POWER = 4;
static const uint32_t GPS_BAUD_RATE = 9600;
static const uint32_t LOC_SEND_INTERVAL_MS = 15000;
static const uint32_t GPS_STATUS_INTERVAL_MS = 30000;
static const uint32_t DISPLAY_UPDATE_INTERVAL_MS = 60000;

// Hardware instances
BluetoothSerial SerialBT;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
XPowersAXP2101 pmu;
TinyGPSPlus gps;
HardwareSerial GPSSerial(1);

// Runtime state
bool pmuReady = false;
bool gpsPowerEnabled = false;

String lastDisplayMessage = "-";
int lastRSSI = 0;

uint32_t messageCounter = 0;
uint32_t locationCounter = 0;

String usbLine;
String btLine;

unsigned long lastLocSendMs = 0;
unsigned long lastGpsStatusMs = 0;
unsigned long lastDisplayUpdateMs = 0;
uint32_t gpsCharsAtLastStatus = 0;

// Message log
static const uint8_t MESSAGE_LOG_SIZE = 20;
String messageLog[MESSAGE_LOG_SIZE];
uint8_t messageLogStart = 0;
uint8_t messageLogCount = 0;
#pragma endregion

void emitLine(const String &line);
void updateDisplay();

#pragma region TTN / LoRaWAN - non-blackout mode

static const u1_t APPEUI[8] PROGMEM = { 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 };
static const u1_t DEVEUI[8] PROGMEM = { 0x26, 0x7D, 0x07, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };
static const u1_t APPKEY[16] PROGMEM = {
  0x42, 0x79, 0x36, 0x4F, 0xBA, 0xA2, 0x4D, 0xFC,
  0xFD, 0x82, 0xC4, 0x71, 0xFD, 0x45, 0x1A, 0x2C
};

const lmic_pinmap lmic_pins = {
  .nss = 18,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 23,
  .dio = { 26, 33, 32 }
};

void os_getArtEui(u1_t *buf) {
  memcpy_P(buf, APPEUI, 8);
}

void os_getDevEui(u1_t *buf) {
  memcpy_P(buf, DEVEUI, 8);
}

void os_getDevKey(u1_t *buf) {
  memcpy_P(buf, APPKEY, 16);
}

void emitTtnStatus(const String &status) {
  emitLine("TTN|" + status);
}

void ttnJoinIfNeeded() {
  if (currentMode != DeviceMode::TTN || ttnJoined || !ttnReady) {
    return;
  }

  LMIC_startJoining();
  emitTtnStatus("JOINING");
}

void sendTTNText(const String &text) {
  if (text.length() == 0) {
    return;
  }

  if (currentMode != DeviceMode::TTN) {
    emitLine("STATUS|TTN|MODE|BLACKOUT_ONLY");
    return;
  }

  String trimmed = text;
  trimmed.trim();
  if (trimmed.length() == 0) {
    return;
  }

  if (trimmed.length() > 50) {
    trimmed = trimmed.substring(0, 50);
  }

  if (!ttnJoined) {
    ttnPendingText = trimmed;
    ttnJoinIfNeeded();
    emitLine("STATUS|TTN|QUEUED|" + trimmed);
    return;
  }

  uint8_t payload[52];
  const size_t len = min((size_t)trimmed.length(), sizeof(payload));
  for (size_t i = 0; i < len; i++) {
    payload[i] = (uint8_t)trimmed.charAt(i);
  }

  LMIC_setTxData2(1, payload, len, 0);
  emitLine("TTN|TX|" + trimmed);
}

void configureTTN() {
  currentMode = DeviceMode::TTN;
  ttnReady = true;
  os_init_ex((const void *)&lmic_pins);
  LMIC_setLinkCheckMode(0);
  LMIC_selectSubBand(1);
  LMIC_setAdrMode(0);
  LMIC_setDrTxpow(DR_SF7, 14);
  emitTtnStatus("READY");
  ttnJoinIfNeeded();
}

void onEvent(ev_t ev) {
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      emitTtnStatus("SCAN_TIMEOUT");
      break;
    case EV_BEACON_FOUND:
      emitTtnStatus("BEACON_FOUND");
      break;
    case EV_BEACON_MISSED:
      emitTtnStatus("BEACON_MISSED");
      break;
    case EV_JOINING:
      emitTtnStatus("JOINING");
      break;
    case EV_JOINED:
      ttnJoined = true;
      emitTtnStatus("JOINED");
      if (ttnPendingText.length() > 0) {
        const String queued = ttnPendingText;
        ttnPendingText = "";
        sendTTNText(queued);
      }
      break;
    case EV_RFU1:
      emitTtnStatus("RFU1");
      break;
    case EV_JOIN_TXCOMPLETE:
      emitTtnStatus("JOIN_TXCOMPLETE");
      break;
    case EV_TXCOMPLETE:
      emitTtnStatus("TX_COMPLETE");
      if (LMIC.txrxFlags & TXRX_ACK) {
        emitTtnStatus("ACK");
      }
      if (LMIC.dataLen > 0) {
        String downlink;
        for (uint8_t i = 0; i < LMIC.dataLen; i++) {
          downlink += (char)LMIC.frame[LMIC.dataBeg + i];
        }
        downlink.trim();
        lastDisplayMessage = downlink;
        updateDisplay();
        emitLine("TTN|DOWNLINK|" + downlink);
      }
      break;
    case EV_LOST_TSYNC:
      emitTtnStatus("LOST_TSYNC");
      break;
    case EV_RESET:
      emitTtnStatus("RESET");
      break;
    case EV_RXCOMPLETE:
      emitTtnStatus("RX_COMPLETE");
      if (LMIC.dataLen > 0) {
        String downlink;
        for (uint8_t i = 0; i < LMIC.dataLen; i++) {
          downlink += (char)LMIC.frame[LMIC.dataBeg + i];
        }
        downlink.trim();
        lastDisplayMessage = downlink;
        updateDisplay();
        emitLine("TTN|DOWNLINK|" + downlink);
      }
      break;
    case EV_LINK_DEAD:
      emitTtnStatus("LINK_DEAD");
      ttnJoined = false;
      break;
    case EV_LINK_ALIVE:
      emitTtnStatus("LINK_ALIVE");
      break;
    default:
      break;
  }
}

void setupTTNMode() {
  if (!ttnReady) {
    configureTTN();
  }
}

#pragma endregion

#pragma region DISPLAY - OLED display

void emitLine(const String &line);
void updateDisplay();

int currentBatteryPercent() {
  if (!pmuReady) {
    return -1;
  }

  const int percent = pmu.getBatteryPercent();
  return percent >= 0 && percent <= 100 ? percent : -1;
}

void updateDisplay() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  const int batteryPercent = currentBatteryPercent();

  display.setCursor(0, 0);
  display.print("Name:");
  display.println(BT_NAME);

  display.setCursor(0, 12);
  display.print("BAT: ");
  if (batteryPercent >= 0) {
    display.print(batteryPercent);
    display.println("%");
  } else {
    display.println("--");
  }

  display.setCursor(0, 24);
  display.print("RSSI: " );
  display.print(lastRSSI);
  display.println("dBm");

  display.setCursor(0, 36);
  display.println("Msg:");

  String msg = lastDisplayMessage;
  if (msg.length() > 40) {
    msg = msg.substring(0, 40);
  }

  display.setCursor(0, 48);
  display.println(msg);

  display.display();
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

#pragma endregion

#pragma region UTILITY - Escaping and string handling

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

#pragma endregion

#pragma region MESSAGE LOG - Message buffer and output

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

#pragma endregion

#pragma region GPS - Power, status, and location

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

void sendLoRaPayload(const String &payload) {
  LoRa.idle();
  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();
  LoRa.receive();
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

  sendLoRaPayload(payload);

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

#pragma endregion

#pragma region LORA - Communication and chat

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

  sendLoRaPayload(payload);

  emitChatLine("TX|" + String(DEVICE_ID) + "|" + String(messageCounter) + "|" + escapedText);

  lastDisplayMessage = text;
  updateDisplay();
}

void handleCommand(String line) {
  line.trim();
  if (line.length() == 0) {
    return;
  }

  if (line == "MODE|TTN") {
    setupTTNMode();
    emitLine("STATUS|MODE|TTN");
    return;
  }

  if (line == "MODE|BLACKOUT") {
    currentMode = DeviceMode::BLACKOUT;
    ttnJoined = false;
    emitLine("STATUS|MODE|BLACKOUT");
    return;
  }

  if (line == "TTN|JOIN") {
    setupTTNMode();
    ttnJoinIfNeeded();
    return;
  }

  if (line.startsWith("TTN|SEND|")) {
    setupTTNMode();
    sendTTNText(line.substring(9));
    return;
  }

  if (line.startsWith("TTN|TXT|")) {
    setupTTNMode();
    sendTTNText(line.substring(8));
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

void emitRawPacket(const String &payload) {
  emitLine("RAW|" + String(LoRa.packetRssi()) + "|" +
           String(LoRa.packetSnr()) + "|" + escapeField(payload));
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
      emitRawPacket(payload);
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
    emitRawPacket(payload);
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

#pragma endregion

#pragma region SETUP & LOOP - Initialization and main loop

void setup() {
  Serial.begin(115200);
  delay(500);

  // Init I2C early: needed for OLED and possible AXP power management.
  Wire.begin(PIN_DISPLAY_SDA, PIN_DISPLAY_SCL);

  pmuReady = pmu.init(Wire, PIN_DISPLAY_SDA, PIN_DISPLAY_SCL, 0x34);
  if (pmuReady) {
    pmu.enableBattDetection();
    pmu.enableBattVoltageMeasure();
  } else {
    emitLine("STATUS|ERROR|AXP2101 init failed");
  }

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
  if (!SerialBT.begin(BT_NAME.c_str())) {
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

  // LoRa / LoRaWAN mode selection
  SPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_SS);

  if (currentMode == DeviceMode::BLACKOUT) {
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
  }

  emitLine("STATUS|OK|Device " + String(DEVICE_ID) + " ready");
}

void loop() {
  const uint32_t now = millis();

  if (currentMode == DeviceMode::TTN) {
    os_runloop_once();
    if (ttnReady && !ttnJoined && now - lastGpsStatusMs >= 15000UL) {
      ttnJoinIfNeeded();
    }
  } else {
    while (GPSSerial.available()) {
      gps.encode((char)GPSSerial.read());
    }

    if (now - lastGpsStatusMs >= GPS_STATUS_INTERVAL_MS) {
      lastGpsStatusMs = now;
      emitGpsStatus();
    }

    if (now - lastLocSendMs >= LOC_SEND_INTERVAL_MS) {
      lastLocSendMs = now;
      sendLocationBroadcast();
    }

    if (now - lastDisplayUpdateMs >= DISPLAY_UPDATE_INTERVAL_MS) {
      lastDisplayUpdateMs = now;
      updateDisplay();
    }

    readCommandStream(Serial, usbLine);
    readCommandStream(SerialBT, btLine);
    handleIncomingLoRa();
  }

  readCommandStream(Serial, usbLine);
  readCommandStream(SerialBT, btLine);
}

#pragma endregion
