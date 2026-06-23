#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WebServer.h>
#include "BluetoothSerial.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// LILYGO T-Beam AXP2101 with SX1276/SX1278.
// Use 868E6 in most of Europe, 915E6 in the US, 433E6 for 433 MHz modules.
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

// Change this before flashing each LilyGO: 1, 2, 3, ...
static const uint8_t DEVICE_ID = 2;
static const char *BT_NAME = "LoraChat-2";
static const char *WIFI_AP_SSID = "LoraChat-2";
static const char *WIFI_AP_PASSWORD = "12345678";

BluetoothSerial SerialBT;
WebServer webServer(80);

uint32_t messageCounter = 0;
String usbLine;
String btLine;

static const uint8_t MESSAGE_LOG_SIZE = 20;
String messageLog[MESSAGE_LOG_SIZE];
uint8_t messageLogStart = 0;
uint8_t messageLogCount = 0;

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="de">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>LoRa Chat</title>
  <style>
    :root{--bg:#eef2f5;--panel:#fff;--line:#d7dee5;--text:#17212b;--muted:#657282;--out:#d8ecff;--in:#f4f7f9;--accent:#1769aa}
    *{box-sizing:border-box}body{margin:0;min-height:100vh;background:var(--bg);font-family:system-ui,-apple-system,Segoe UI,sans-serif;color:var(--text)}
    main{min-height:100vh;display:grid;place-items:center;padding:14px}.panel{width:min(760px,100%);height:min(760px,calc(100vh - 28px));display:grid;grid-template-rows:auto 1fr auto;background:var(--panel);border:1px solid var(--line);border-radius:8px;overflow:hidden}
    header{padding:16px;border-bottom:1px solid var(--line)}h1{font-size:1.25rem;margin:0}p{margin:4px 0 0;color:var(--muted)}
    #messages{padding:14px;overflow:auto;background:#fbfcfd}.msg{max-width:86%;margin:0 0 10px;padding:9px 11px;border:1px solid var(--line);border-radius:8px;background:var(--in);overflow-wrap:anywhere;white-space:pre-wrap}.tx{margin-left:auto;background:var(--out);border-color:#b9d8f2}.meta{display:block;margin-bottom:4px;color:var(--muted);font-size:.78rem}
    form{display:grid;grid-template-columns:1fr auto;gap:8px;padding:12px;border-top:1px solid var(--line)}input,button{font:inherit;padding:10px;border-radius:8px;border:1px solid var(--line)}button{min-width:92px;border-color:var(--accent);background:var(--accent);color:white}
    @media(max-width:560px){main{padding:0}.panel{height:100vh;border:0;border-radius:0}form{grid-template-columns:1fr}button{width:100%}}
  </style>
</head>
<body>
  <main>
    <section class="panel">
      <header><h1>LoRa Chat</h1><p>Verbunden mit 192.168.4.1</p></header>
      <div id="messages"></div>
      <form id="form"><input id="text" maxlength="180" placeholder="Nachricht eingeben..." autocomplete="off"><button>Senden</button></form>
    </section>
  </main>
  <script>
    const box=document.getElementById('messages'),form=document.getElementById('form'),input=document.getElementById('text');
    let last='';
    function esc(s){return String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));}
    async function load(){
      const text=await fetch('/api/messages',{cache:'no-store'}).then(r=>r.text()).catch(()=>null);
      if(text===null||text===last)return; last=text;
      box.innerHTML=text.split('\n').filter(Boolean).map(line=>{
        const tx=line.startsWith('TX|'), parts=line.split('|'), msg=parts.slice(tx?3:5).join('|');
        const meta=tx?`Gesendet von LilyGO ${parts[1]}, Paket ${parts[2]}`:`LilyGO ${parts[1]}, RSSI ${parts[3]}, SNR ${parts[4]}`;
        return `<div class="msg ${tx?'tx':'rx'}"><span class="meta">${esc(meta)}</span>${esc(msg)}</div>`;
      }).join('') || '<p>Noch keine Nachrichten.</p>';
      box.scrollTop=box.scrollHeight;
    }
    form.addEventListener('submit',async e=>{
      e.preventDefault(); const text=input.value.trim(); if(!text)return;
      input.value=''; await fetch('/api/send',{method:'POST',headers:{'Content-Type':'text/plain'},body:text}).catch(()=>{});
      load();
    });
    load(); setInterval(load,1000);
  </script>
</body>
</html>
)HTML";

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

void emitLine(const String &line) {
  Serial.println(line);
  if (SerialBT.hasClient()) {
    SerialBT.println(line);
  }
}

void emitChatLine(const String &line) {
  addMessageLog(line);
  emitLine(line);
}

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
