# BlackoutBuddy ESP Firmware

Diese Firmware läuft auf einem ESP32-T-Beam und bildet die Hardware- und Kommunikationsschicht für BlackoutBuddy. Der Smartphone verbindet sich per Bluetooth Classic (SPP) mit dem Knoten; danach kann die App zwischen zwei Betriebsmodi wählen:

- Blackout: direkte Knoten-zu-Knoten-Kommunikation über LoRa
- TTN / Non-Blackout: LoRaWAN-OTAA-Join mit The Things Network (TTN)

Es gibt keinen eigenen WLAN-, HTTP- oder WebSocket-Backendpfad. Die TTN-Kommunikation läuft direkt in der Firmware.

---

## Hardware- und Plattformstatus

### Unterstütztes Board

- LilyGO T-Beam mit ESP32
- LoRa-Modul SX1276/SX1278
- OLED-Display über I2C
- GPS-Modul mit Power-Steuerung

### Wichtiges in `src/main.cpp`

Jeder Knoten hat eine eindeutige `DEVICE_ID`, die auch als Teil des Bluetooth-Namens genutzt wird:

```cpp
static const uint8_t DEVICE_ID = 1;
static const char *BT_NAME_BASE = "BlackoutBuddy";
static String BT_NAME = String(BT_NAME_BASE) + "-" + String(DEVICE_ID);
```

Damit ergeben sich Namen wie:

- `BlackoutBuddy-1`
- `BlackoutBuddy-2`

Diese IDs müssen für jedes Gerät eindeutig sein.

---

## LoRa-Konfiguration

Die Firmware verwendet feste gemeinsame LoRa-Parameter für die Blackout-Kommunikation. Alle Knoten im selben Test-Setup sollten dieselbe Konfiguration nutzen:

- Frequenz: `868E6` (EU-868) bzw. passend zur Region
- Pins:
  - SCK = 5
  - MISO = 19
  - MOSI = 27
  - NSS = 18
  - RST = 23
  - DIO0 = 26

Der Betrieb mit Antenne ist zwingend erforderlich. Ohne passende Antenne oder mit inkonsistenter Frequenzkonfiguration ist ein stabiler LoRa-Link nicht garantiert.

---

## Moduslogik

Die Firmware definiert zwei Modi:

```cpp
enum class DeviceMode {
  BLACKOUT,
  TTN
};
```

- `BLACKOUT`: LoRa-Knotenkommunikation, ohne Internet
- `TTN`: OTAA-Join zu TTN und Text-Downlink/Uplink-Handling

Die App schickt dafür Befehle wie:

- `MODE|BLACKOUT`
- `MODE|TTN`

---

## Blackout-Modus

Im Blackout-Modus verarbeitet die Firmware Befehle im Format:

- `CHAT|Text`

Die Firmware verschickt lokale Bestätigungen und LoRa-Empfangsereignisse in Form von Zeilen wie:

- `TX|id|counter|Text`
- `RX|sender|counter|rssi|snr|Text`
- `STATUS|...`

Pipes und Backslashes im Text werden im Protokoll entsprechend escaped, damit die Zeilen eindeutig und robust parsbar bleiben.

### Blackout-Pflichtanforderungen

Für ein Zwei-Geräte-Setup müssen die Knoten:

- unterschiedliche `DEVICE_ID`-Werte haben
- denselben LoRa-Frequenzbereich und dieselben LoRa-Parameter teilen
- direkt per Bluetooth gekuppelt werden
- ebenfalls im selben gemeinsamen LoRa-Setup betrieben werden

---

## TTN / Non-Blackout-Modus

Die Firmware enthält einen TTN-Stack mit OTAA.

### Konfiguration

Vor dem Flashen müssen die Keys passend zur TTN-Anwendung gesetzt werden:

```cpp
static const u1_t APPEUI[8] PROGMEM = { ... };
static const u1_t DEVEUI[8] PROGMEM = { ... };
static const u1_t APPKEY[16] PROGMEM = { ... };
```

Die TTN-Konfiguration muss mit der regionalen Frequenz und der Device-Registrierung im TTN-Backend übereinstimmen.

### Verhalten

- App sendet `MODE|TTN`
- Firmware initialisiert LMIC und versucht OTAA-Join
- `TTN|JOINING` und `TTN|JOINED` werden als Statuszeilen ausgegeben
- Text-Uplinks bis 50 Zeichen werden per `TTN|SEND|...` gesendet
- Downlinks werden als `TTN|DOWNLINK|...` zurück an die App gemeldet

Die Firmware meldet auch allgemeinere Statuslinien wie:

- `TTN|READY`
- `TTN|JOINING`
- `TTN|JOINED`
- `TTN|TX_COMPLETE`
- `TTN|ACK`

---

## GPS für Kartenpositionen

Die Firmware setzt das GPS-Power-Pin beim Start aktiv und verwendet das GPS-Modul für Positionen.

### GPS-Pins

- RX: GPIO 34
- TX: GPIO 12
- Power: GPIO 4
- Baud: 9600

### Typische GPS-Statuszeilen

- `STATUS|GPS|NO_DATA|check_power_or_pins`
- `STATUS|GPS|NO_FIX|...`
- `STATUS|GPS|FIX|lat|...|lon|...`

Nur bei einem gültigen Fix werden Positionsdaten weiterverarbeitet.

### GPS-Kommandos

Über USB oder Bluetooth können zusätzlich folgende Befehle verwendet werden:

- `GPS|STATUS` -> sofortige Statusausgabe
- `GPS|POWERON` -> GPS-Power erneut aktivieren

Positionen werden als:

- `LOC|nodeId|lat|lon|accuracy|timestamp`

gesendet. Das Zeitformat ist UTC im ISO-Format:

- `YYYY-MM-DDTHH:MM:SSZ`

---

## Build und Upload

```powershell
cd esp
pio run -e t-beam -t upload
pio device monitor -e t-beam
```

Danach kann der Knoten in Android als `BlackoutBuddy-*` gekoppelt werden. Die App verbindet sich mit dem Gerät und zeigt anschließend die Modusauswahl.

---

## Wichtige Hinweise für den Betrieb

- Jeder Knoten muss eine eindeutige `DEVICE_ID` haben.
- Alle Knoten im Blackout-Setup müssen dieselbe LoRa-Region und dieselben LoRa-Parameter verwenden.
- Für TTN muss die OTAA-Konfiguration zu dem gewählten Device im TTN-Backend passen.
- Die Firmware arbeitet ohne zentrales Backend; die App spricht direkt über Bluetooth mit dem ESP32.

---

## Projektkontext

Der aktuelle Firmware-Stack entspricht dem realen Projektstatus:

- Bluetooth Classic als primäres Kontrollmedium zur App
- LoRa als primärer Peer-to-Peer-Transport im Blackout-Modus
- LoRaWAN/TTN als optionaler Zusatzpfad im Non-Blackout-Modus
- GPS und Statusausgaben als Zusatzfunktionen für Karten- und Diagnostikansichten

Damit ist die Firmware bereits auf den aktuellen BlackoutBuddy-Implementierungsumfang abgestimmt und nicht mehr nur auf den ursprünglichen Minimal-Plan.

