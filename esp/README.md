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

## Serielles Protokoll

Vom PC zum LilyGO:

```text
CHAT|Hallo Welt
```

Vom LilyGO zum PC:

```text
STATUS|OK|DEVICE|1|BT|LoraChat-1
STATUS|OK|WIFI|LoraChat-1|IP|192.168.4.1
TX|1|7|Hallo Welt
RX|2|4|-72|8.25|Antwort vom zweiten LilyGO
RAW|-80|7.50|Unbekannte Payload
```

`TX` bestaetigt, dass das lokale Geraet gesendet hat. `RX` ist eine ueber LoRa
empfangene Chatnachricht von einem anderen Geraet.
