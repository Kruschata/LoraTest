# ESP Firmware fuer LilyGO T-Beam SX1276

Diese Firmware macht aus dem LilyGO T-Beam einen einfachen LoRa-Chat-Knoten.
Der PC spricht per USB-Serial oder Bluetooth-Serial mit dem LilyGO. Die LilyGOs
sprechen untereinander ausschliesslich ueber LoRa-Funk.
Zusaetzlich startet jeder LilyGO einen eigenen WLAN Access Point mit einer
kleinen Chat-Website unter `http://192.168.4.1`.

## Hardware

- LilyGO T-Beam mit ESP32, AXP2101 und SX1276/SX1278
- Antenne muss vor dem Senden angeschlossen sein
- Frequenz im Sketch passend zum Modul/Land setzen:
  - `868E6` fuer die meisten EU-868-Module
  - `915E6` fuer US-915-Module
  - `433E6` fuer 433-MHz-Module

Die verwendeten LoRa-Pins sind:

| Signal | GPIO |
| --- | ---: |
| SCK | 5 |
| MISO | 19 |
| MOSI | 27 |
| NSS/SS | 18 |
| RST | 23 |
| DIO0 | 26 |

## Vor dem Flashen

In `src/main.cpp` pro Geraet eindeutig setzen:

```cpp
static const uint8_t DEVICE_ID = 1;
static const char *BT_NAME = "LoraChat-1";
static const char *WIFI_AP_SSID = "LoraChat-1";
static const char *WIFI_AP_PASSWORD = "12345678";
```

Beispiel:

- Geraet A: `DEVICE_ID = 1`, `BT_NAME = "LoraChat-1"`, `WIFI_AP_SSID = "LoraChat-1"`
- Geraet B: `DEVICE_ID = 2`, `BT_NAME = "LoraChat-2"`, `WIFI_AP_SSID = "LoraChat-2"`

Alle Geraete muessen dieselbe Frequenz und dieselben LoRa-Parameter verwenden.

## LoRa-Konfiguration

Die LoRa-Parameter sind in der `setup()`-Funktion konfiguriert:

| Parameter | Wert | Erklärung |
|-----------|------|-----------|
| **Synchronisationswort** | `0x34` | Geheimschlüssel - nur Geräte mit gleichem Wort können sich verstehen |
| **Spreizungsfaktor** | `7` | SF7 = schnelle Übertragung, kurze Reichweite; SF12 = langsam, große Reichweite |
| **Bandbreite** | `125 kHz` | Standard; höhere Werte ermöglichen schnellere Übertragung |
| **Fehlerkorrektur** | `4/5` | 80% Fehlertoleranz; höhere Werte = robuster aber langsamer |
| **Sendeleistung** | `17 dBm` | ~50 mW; höher = größere Reichweite, aber mehr Stromverbrauch |
| **CRC** | aktiviert | Fehlerprüfung: fehlerhafte Pakete werden verworfen |

**Wichtig:** Alle Geräte im Netzwerk müssen die **gleichen Parameter** haben, sonst können sie nicht kommunizieren!

## Direkt per WLAN verbinden

Nach dem Flashen startet der LilyGO ein WLAN:

```text
SSID: LoraChat-1 bzw. LoraChat-2
Passwort: 12345678
Adresse: http://192.168.4.1
```

So benutzt du es:

1. Mit Handy, Tablet oder PC mit dem WLAN des LilyGO verbinden.
2. Browser oeffnen und `http://192.168.4.1` aufrufen.
3. Nachricht eingeben und senden.

Diese Website laeuft direkt auf dem ESP32. Es ist kein Node-Server noetig.
Empfangene LoRa-Nachrichten werden automatisch nachgeladen.

## Build und Upload

In VS Code mit PlatformIO:

1. Ordner `esp` oeffnen.
2. PlatformIO Environment `t-beam` auswaehlen.
3. `Build` ausfuehren.
4. `Upload` ausfuehren.

Per CLI, falls `pio` installiert ist:

```powershell
cd C:\Users\Lukas\Desktop\LoraTest-1\esp
pio run -e t-beam -t upload
pio device monitor -e t-beam
```

Hinweis: Das Projekt nutzt `huge_app.csv`, damit LoRa, Bluetooth und WLAN-Webserver
gemeinsam in den Flash passen. OTA-Updates sind damit nicht vorgesehen.

## Programmlogik

### `sendChatMessage(String text)`
Sendet eine Chatnachricht über LoRa:
- Trimmt und validiert den Text (max. 180 Zeichen)
- Erhöht einen Message-Zähler
- Escapiert Sonderzeichen (`\` und `|`)
- Baut ein Payload-Format: `LCHAT|1|DEVICE_ID|counter|escapedText`
- Sendet das Paket über LoRa
- Gibt eine TX-Meldung aus und zeigt die Nachricht auf dem Display

### `handleCommand(String line)`
Verarbeitet eingehende Befehle:
- Wenn die Zeile mit `CHAT|` beginnt → Text danach als Nachricht senden
- Sonst → ganze Zeile als Nachricht senden

### `readCommandStream(Stream &stream, String &buffer)`
Liest Daten aus USB oder Bluetooth:
- Liest Zeichen für Zeichen
- Bei Zeilenumbruch (`\n`) → `handleCommand()` aufrufen
- Ignoriert Carriage Returns (`\r`)
- Maximale Buffer-Länge: 220 Zeichen

### `handleIncomingLoRa()`
Verarbeitet eingehende LoRa-Pakete:
- Parst das Payload-Format: findet die Pipe-Trennzeichen (`|`)
- Bei **ungültigem Format** → sendet RAW-Daten mit RSSI/SNR
- Bei **gültigem Format** → extrahiert Sender-ID, Counter und Text
- Ignoriert Nachrichten vom **eigenen Gerät**
- Emittiert RX-Nachricht mit Sender-ID, Counter, RSSI, SNR und Text

## Serielles Protokoll

Vom PC zum LilyGO:


```

`TX` bestaetigt, dass das lokale Geraet gesendet hat. `RX` ist eine ueber LoRa
empfangene Chatnachricht von einem anderen Geraet.
