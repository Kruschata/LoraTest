# BlackoutBuddy - Implementierungsplan und Projektstruktur

## Ziel des Projekts

BlackoutBuddy ist ein lokales Kommunikationsprojekt bestehend aus einer Flutter-App und ESP32-/T-Beam-Firmware. Der Fokus liegt auf einer dezentralen, serverlosen Kommunikation zwischen Geräten, mit zwei Betriebsmodi:

1. Smartphone <-> ESP32/T-Beam per Bluetooth Classic (SPP)
2. ESP32/T-Beam <-> ESP32/T-Beam per LoRa im Blackout-Modus
3. ESP32/T-Beam -> TTN per LoRaWAN im Non-Blackout-Modus

Es gibt aktuell keinen eigenen WebSocket-, HTTP- oder Cloud-Backendpfad. TTN wird direkt von der ESP-Firmware über LoRaWAN/OTAA angesprochen.

---

## Aktueller Projektstruktur

```text
LoraTest/
├── app/
│   ├── README.md
│   ├── analysis_options.yaml
│   ├── pubspec.yaml
│   ├── android/
│   ├── lib/
│   │   ├── main.dart
│   │   ├── models/
│   │   │   ├── device.dart
│   │   │   └── message.dart
│   │   ├── screens/
│   │   │   ├── blackoutMode_connection_screen.dart
│   │   │   ├── connection_mode_screen.dart
│   │   │   ├── device_map_screen.dart
│   │   │   ├── device_status_screen.dart
│   │   │   └── non_blackout_home_screen.dart
│   │   └── services/
│   │       ├── bluetooth_service.dart
│   │       └── node_registry_service.dart
│   └── third_party/
│       └── flutter_bluetooth_serial_plus/
├── esp/
│   ├── README.md
│   ├── platformio.ini
│   └── src/
│       └── main.cpp
├── BLACKOUTBUDDY_IMPLEMENTATION_PLAN.md
├── plan.md
└── .gitignore
```

## Relevante Bestandteile

### Flutter-App (`app/`)

Die App ist der Nutzer- und Steuerungsbereich für die Geräteverbindung.

- `app/lib/main.dart`: App-Startpunkt und Einstieg in die UI
- `app/lib/screens/connection_mode_screen.dart`: Auswahl zwischen Blackout- und Non-Blackout-Modus
- `app/lib/screens/blackoutMode_connection_screen.dart`: Blackout-Chat-UI
- `app/lib/screens/non_blackout_home_screen.dart`: TTN-/Gateway-Ansicht mit Status und Utility-Panel
- `app/lib/services/bluetooth_service.dart`: Bluetooth Classic-Verbindung zum ESP32 mit SPP-Handling und Linienparser
- `app/lib/services/node_registry_service.dart`: lokale Knoten-/Netzwerkverwaltung
- `app/lib/models/device.dart` und `app/lib/models/message.dart`: Datenmodelle für Geräte und Nachrichten

### ESP-Firmware (`esp/`)

Die Firmware läuft auf einem ESP32/T-Beam und übernimmt die Hardware-zu-Hardware-Kommunikation.

- `esp/src/main.cpp`: zentrale Logik für LoRa, Bluetooth, TTN, GPS und Moduswechsel
- `esp/README.md`: Hardware-, Flash- und TTN-Setup
- `platformio.ini`: Build- und Upload-Konfiguration

### Third-Party-Integration

- `app/third_party/flutter_bluetooth_serial_plus/`: eigener Bluetooth-Wrapper für die Android-Integration und die RFCOMM-Verbindung

---

## Aktueller Ist-Stand

- Die Flutter-App verbindet sich zunächst per Bluetooth Classic mit einem gekoppelten BlackoutBuddy-Knoten.
- Nach der Verbindung bietet die App die Auswahl zwischen `Blackout Mode` und `Non-Blackout Mode` an.
- Im Blackout-Modus werden LoRa-Nachrichten via `CHAT|...` und Statuslinien wie `TX|...`, `RX|...`, `STATUS|...` verarbeitet.
- Im Non-Blackout-Modus wird ein TTN-Flow über `MODE|TTN`, `TTN|SEND|...` und `TTN|JOINED` / `TTN|DOWNLINK|...` unterstützt.
- Die ESP-Firmware enthält ebenfalls GPS-Unterstützung und meldet Positionen als `LOC|...`.
- Die App besitzt bereits getrennte UI-Pfade für Blackout und TTN-Ansicht.

---

## Kommunikationsmodell

1. Nutzer sendet in der App eine Nachricht.
2. App sendet diese per Bluetooth an den verbundenen ESP32.
3. Der ESP32 verarbeitet den Modus:
   - Blackout: weiterleiten via LoRa an den zweiten Knoten
   - TTN: LoRaWAN-Uplink an TTN senden
4. Empfangene Daten werden lokal als Status-/Nachrichtenzeilen an die App zurückgegeben.

Wesentliche Protokollmuster:

- `MODE|BLACKOUT`
- `MODE|TTN`
- `CHAT|Text`
- `TTN|SEND|Text`
- `TX|id|counter|Text`
- `RX|sender|counter|rssi|snr|Text`
- `STATUS|...`
- `LOC|nodeId|lat|lon|accuracy|timestamp`

---

## Technische Anforderungen für ein Zwei-Geräte-Setup

Für einen funktionierenden Zwei-Knoten-Betrieb gelten die folgenden Voraussetzungen:

- Eindeutige `DEVICE_ID` pro ESP32/Knoten in `esp/src/main.cpp`
- Eindeutiger Bluetooth-Name pro Knoten, z. B. `BlackoutBuddy-1`, `BlackoutBuddy-2`
- Identische LoRa-Parameter auf allen Knoten:
  - Frequenz
  - Sync Word
  - Spreading Factor
  - Bandwidth
  - Coding Rate
- Gleiche regionale Konfiguration für TTN-Join, falls Non-Blackout-Modus genutzt wird
- Android: Geräte vorab koppeln, inklusive eventuellem PIN, z. B. `1234`

---

## Aktueller Scope und Grenzen

### In Scope

- Smartphone <-> ESP32 mit Bluetooth Classic
- ESP32 <-> ESP32 über LoRa im Blackout-Modus
- Optionaler TTN-Pfad im Non-Blackout-Modus
- Lokale Bedienung ohne Backend
- App-/Firmware-Statuslinien für Diagnose und Debugging

### Bekannte Grenzen

- Kein Multi-Hop-Relay zwischen mehreren Knoten
- Kein Routing oder Zieladressierung für größere Mesh-Topologien
- Keine Store-and-Forward-Strategie auf Routing-Ebene
- Keine zentrale Server-Komponente für Messaging oder Device-Management
- GPS-Positionen sind zusätzliches Feature und keine Voraussetzung für die Grundfunktion

---

## Nächste Ausbaustufen

1. Nachrichten-Deduplizierung über Sender + Counter
2. TTL/Hop-Count im LoRa-Payload ergänzen
3. Optionales Relay für einfache Multi-Hop-Netze
4. Zieladressierung (Unicast/Broadcast) einführen
5. ACK-/Retry-Mechanik für robustere Zustellung
6. TTN-Status und Node-Health weiter ausbauen
7. Karten- und Geräte-Logik auf reale Nutzung validieren

---

## Definition of Done

### Blackout-Funktionalität

- Zwei T-Beams sind erfolgreich geflasht und individuell konfiguriert.
- Beide Knoten sind in Android als Bluetooth-Geräte sichtbar und koppelfähig.
- Nachrichten von Gerät A erscheinen auf Gerät B per LoRa.
- Nachrichten von Gerät B erscheinen auf Gerät A per LoRa.

### Non-Blackout-/TTN-Funktionalität

- Gerät kann per Bluetooth verbunden werden.
- Moduswechsel `MODE|TTN` wird korrekt verarbeitet.
- TTN-Join funktioniert mit gültigem Device-Key/OTAA-Konfiguration.
- Uplinks werden gesendet und Downlinks werden in der App angezeigt.

### Projekt-Umfang

- Keine Server-/WebSocket-/HTTP-Backend-Komponente ist für den Betrieb erforderlich.
- Die App und Firmware bleiben auf die lokale Hardware- und LoRa-Architektur fokussiert.

---

## Empfehlung für die weitere Umsetzung

Die aktuelle Struktur des Projekts ist bereits konsistent mit einem Zwei-Layer-Ansatz:

- App als Benutzeroberfläche und Protokollsteuerung
- ESP32 als lokaler Kommunikations- und Hardware-Knoten
- LoRa als primäres Transportmedium im Blackout-Mode
- TTN als optionaler zusätzlicher Pfad im Non-Blackout-Mode

Der nächste Schritt ist die Verifikation der realen Konnektivität und der Moduswechsel auf echten Hardware-Knoten, bevor weitere Routing- oder Backend-Funktionen ergänzt werden.
