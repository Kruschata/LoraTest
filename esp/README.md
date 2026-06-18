ESP (Lilygo TTGO LORA32) — Anleitung

Kurz: Dieses Projekt verwendet PlatformIO (VSCode) zum Kompilieren und Flashen.

Voraussetzungen
- PlatformIO (VSCode Extension) oder PlatformIO Core (CLI)
- USB-Treiber für das ESP32-Board installiert

Wichtig
- In `src/main.cpp` gibt es `#define DEVICE_ID 1`. Ändere diese ID für jedes Gerät (z. B. 1, 2, 3...).

Build & Upload (PlatformIO in VSCode)
1. Öffne das Projekt-Ordner `esp` in VSCode.
2. Öffne die PlatformIO-Leiste und wähle das Environment `esp32doit-devkit-v1`.
3. Klicke auf `Build` und danach `Upload` (oder `Upload and Monitor`).

Alternativ CLI:
```bash
# vom Ordner esp
pio run -e esp32doit-devkit-v1 -t upload
pio device monitor -b 115200
```

Testen
- Nach dem Upload öffne den Seriellen Monitor (`115200` Baud).
- Verbinde den Lilygo per USB mit dem Server-PC (dieser PC hostet `server/server.js`).
- Starte den Node-Server (siehe `server/README.md`) und öffne die Website.
- Sende Texte über die Website — sie erscheinen im seriellen Monitor und werden per LoRa übertragen.
- Andere LoRa-Geräte empfangen die Nachricht und antworten; die Antwort erscheint dann ebenfalls im Web-UI.

OLED-Anzeige
- Wenn dein Lilygo ein OLED (0.96" / 128x64, I2C 0x3C) hat, zeigt das Gerät jetzt gesendete und empfangene Nachrichten auf dem OLED an.
- Beim Senden: auf dem Sender erscheint "Sende:" + Payload.
- Beim Empfangen: auf dem Empfänger erscheint "Empfangen:" + Payload.
- Länge wird für Anzeige gekürzt; vollständige Daten stehen im seriellen Monitor.

Format (einfaches Protokoll)
- Ausgehende Nachrichten vom Web-UI sollten optional ein Ziel angeben: `TO:<targetId>|Dein Text`.
- Der Sketch hängt automatisch `FROM:<DEVICE_ID>|` an die LoRa-Payload an.
- Wenn ein Gerät eine LoRa-Payload empfängt, leitet es diese unverändert per Serial an den Server weiter.

Wenn du möchtest, kann ich das Protokoll auf JSON umbauen, zusätzliche Felder (Zeitstempel, RSSI, etc.) hinzufügen oder eine einfache Adresse-/Routing-Logik implementieren.