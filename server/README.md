# LoRa Chat Server

Der Server verbindet die Website mit einem LilyGO T-Beam. Die Website sendet per
WebSocket an Node.js, Node.js sendet per Serial an den LilyGO, und der LilyGO
sendet die Nachricht per LoRa an die anderen LilyGOs.

## Start

```powershell
cd C:\Users\Lukas\Desktop\LoraTest-1\server
npm install
$env:SERIAL_PORT='COM11'
$env:SERIAL_BAUD='115200'
npm start
```

Danach im Browser oeffnen:

```text
http://localhost:5500
```

## COM-Port finden

Wenn du nicht weisst, welcher COM-Port der LilyGO ist:

```powershell
cd C:\Users\Lukas\Desktop\LoraTest-1\server
node -e "require('serialport').SerialPort.list().then(console.log)"
```

Bei Bluetooth wird der gekoppelte LilyGO ebenfalls als COM-Port angezeigt. Dann
einfach diesen Port in `SERIAL_PORT` eintragen.

## Mehrere PCs / mehrere LilyGOs

Jeder PC startet seinen eigenen Server und verbindet sich mit seinem lokalen
LilyGO. Die Browser/Server sprechen nicht miteinander. Die Kommunikation zwischen
den Geraeten laeuft nur ueber LoRa.

Beispiel:

- PC A -> COM11 -> LilyGO 1
- PC B -> COM7 -> LilyGO 2
- Text von PC A geht ueber Serial zu LilyGO 1, dann per LoRa zu LilyGO 2, dann
  ueber Serial zu PC B und erscheint dort im Chat.

## Nuetzliche Umgebungsvariablen

| Variable | Standard | Bedeutung |
| --- | --- | --- |
| `PORT` | `5500` | HTTP/WebSocket-Port |
| `SERIAL_PORT` | `COM11` | USB- oder Bluetooth-COM-Port |
| `SERIAL_BAUD` | `115200` | Baudrate der Firmware |

## Troubleshooting

- `Serialport konnte nicht geoeffnet werden`: falscher COM-Port, Treiber fehlt,
  Port ist bereits im seriellen Monitor offen, oder Bluetooth ist nicht verbunden.
- Website zeigt `Serial ist nicht verbunden`: Server laeuft, aber der LilyGO-Port
  konnte nicht geoeffnet werden.
- Keine LoRa-Nachrichten: Frequenz, LoRa-Modulversion, Antenne und `DEVICE_ID`
  pruefen. Beide Geraete muessen dieselben LoRa-Parameter haben.
