# Server — Startanleitung & Code-Erklärung

## Kurzüberblick

Der Node.js-Server verbindet einen seriellen LoRa-/ESP32-Port mit WebSocket-Clients (z. B. Browser). Seriell eingehende Textzeilen werden an alle verbundenen WebSocket-Clients weitergeleitet; Nachrichten von Clients werden an das serielle Gerät gesendet.

## Voraussetzungen

- Node.js (LTS empfohlen)
- ESP32 / LoRa-Gerät per USB angeschlossen

## Schnellstart (PowerShell)

`powershell
cd "C:\Users\22762857.HTL\Desktop\LoraTest\server"
npm install
npm start
`

Wichtig: Prüfe vor dem Start in server.js, welcher COM-Port verwendet wird (Default: COM44). Falls nötig, setze die Umgebungsvariable SERIAL_PORT oder passe die Datei an.

## Konfiguration per Umgebungsvariablen

Der Server unterstützt folgende Umgebungsvariablen:

- SERIAL_PORT — z. B. COM3 (Standard: COM44)
- SERIAL_BAUD — Baudrate, z. B. 115200 (Standard: 115200)

PowerShell-Beispiel:

`powershell
cd "C:\Users\22762857.HTL\Desktop\LoraTest\server"
 = "COM3"
 = "115200"
npm start
`

## Server-Code (Kurz erklärt)

**package.json**
- Listet Abhängigkeiten (z. B. serialport, ws) und das start-Script (
ode server.js).

**server.js — Wichtige Bestandteile**
- Importiert ws (WebSocket) und serialport.
- Erstellt einen WebSocket-Server, der auf Port 8080 lauscht.
- Öffnet eine serielle Verbindung zum ESP32/LoRa (Pfad & Baudrate konfigurierbar).
- Empfängt serielle Daten, puffert sie zeilenweise (bis \n) und sendet jede vollständige Zeile an alle verbundenen WebSocket-Clients.
- Empfängt Nachrichten von WebSocket-Clients und leitet sie per port.write(...) an das serielle Gerät weiter.
- Wichtige Events: port.on('open'), port.on('error'), wss.on('connection'), port.on('data').

## ESP32 / LoRa — kurze Erklärung

- Das ESP32/LoRa-Gerät ist für die LoRa-Kommunikation zuständig und sendet/empfängt über USB serielle Texte (Zeilen mit \n).
- Auf dem ESP läuft ein Sketch/Firmware, die serielle Ausgaben formatiert und eingehende Befehle verarbeitet.

## Website (Client) — kurze Erklärung

- Die Website verbindet sich per WebSocket zu ws://<server-host>:8080.
- Nachrichten von der Website werden an das serielle Gerät gesendet; serielle Empfangszeilen werden an die Website weitergeleitet.

## Fehler & Troubleshooting

- Fehler: Opening Com44: file not found
  - Mögliche Ursachen: Gerät nicht angeschlossen, falscher COM-Port oder fehlende Treiber.
  - Prüfe Windows Device Manager → Ports (COM & LPT).
  - PowerShell zum Auflisten der Ports:

    `powershell
    [System.IO.Ports.SerialPort]::GetPortNames()
    # oder
    Get-CimInstance Win32_PnPEntity | Where-Object { .Name -match 'COM' } | Select-Object Name
    `

- Wenn du SERIAL_PORT nutzt, musst du die Datei nicht ändern.

## Logs / Verifikation

- Starte 
pm start und beobachte die Konsole. Erwartete Meldungen:
  - Serialport offen: (seriell verbunden)
  - Client verbunden (bei WebSocket-Verbindungen)
  - Von LoRa: (empfangene Zeilen vom Gerät)

## Weiteres

Wenn du möchtest, passe ich die Website so an, dass die Serveradresse konfigurierbar ist, oder erweitere die Anzeige um den Verbindungsstatus der seriellen Schnittstelle.
