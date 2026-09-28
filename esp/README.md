# BlackoutBuddy ESP Firmware

Firmware für einen LilyGO T-Beam mit ESP32 und SX1276/SX1278. Das Handy
verbindet sich per Bluetooth Classic (SPP) mit einem Knoten. Danach wählt die
App den Betriebsmodus:

- Blackout: Knoten kommunizieren direkt untereinander über LoRa.
- Non-Blackout: Der Knoten verbindet sich per LoRaWAN/OTAA mit The Things
	Network (TTN); die App kann Uplinks senden und Downlinks anzeigen.

Es gibt keinen eigenen WLAN-, HTTP- oder WebSocket-Backendpfad.

## Vor dem Flashen

In `src/main.cpp` jedem Knoten eine eindeutige ID geben. Der Bluetooth-Name
wird automatisch aus der ID gebildet:

```cpp
static const uint8_t DEVICE_ID = 1;
// Bluetooth-Name: BlackoutBuddy-1
```

Alle Knoten müssen dieselbe LoRa-Frequenz und dieselben LoRa-Parameter
verwenden. `868E6` ist passend für EU-868, `915E6` für US-915 und `433E6` für
433-MHz-Module. Vor dem Senden immer eine passende Antenne anschließen.

Die verwendeten Pins sind SCK 5, MISO 19, MOSI 27, NSS 18, RST 23 und DIO0 26.

## GPS fuer Kartenposition

Fuer die Kartenposition sendet die Firmware `LOC`-Zeilen an die App. Auf vielen
T-Beam-Boards muss das GPS-Modul aktiv eingeschaltet werden. Die Firmware setzt
deshalb beim Start den GPS-Power-Pin auf HIGH.

Aktueller GPS-UART in `src/main.cpp`:

- RX: GPIO 34
- TX: GPIO 12
- Baud: 9600

Falls dein Board eine andere GPS-Verdrahtung hat, muessen diese Pins angepasst
werden.

### Diagnostik

Im Monitor erscheinen regelmaessig Statuszeilen:

- `STATUS|GPS|NO_DATA|check_power_or_pins`
- `STATUS|GPS|NO_FIX|...`
- `STATUS|GPS|FIX|lat|...|lon|...`

Nur bei `FIX` werden nutzbare Positionen gesendet. Ohne Fix bleibt die
Kartenansicht in der App leer.

Du kannst die GPS-Diagnose aktiv triggern (USB oder Bluetooth):

- `GPS|STATUS` -> sofortige GPS-Statuszeile
- `GPS|POWERON` -> GPS-Power erneut aktivieren

Die LOC-Zeit nutzt UTC im ISO-Format:

- `YYYY-MM-DDTHH:MM:SSZ`
- Beispiel: `2026-07-30T14:23:05Z`

## Build und Upload

```powershell
cd esp
pio run -e t-beam -t upload
pio device monitor -e t-beam
```

Danach den Knoten in Android als `BlackoutBuddy-*` koppeln. Die App verbindet
sich zuerst mit diesem Gerät und zeigt anschließend die Modusauswahl.

## TTN Non-Blackout-Modus

Die Firmware verwendet OTAA. Vor dem Flashen müssen `APPEUI`, `DEVEUI` und
`APPKEY` in `src/main.cpp` zu einem TTN-Device passen. TTN-Anwendung und
Firmware müssen dieselbe regionale Frequenzkonfiguration verwenden. Der
TTN-Modus wird nach der Bluetooth-Verbindung in der App ausgewählt; die App
sendet dann `MODE|TTN` an den Knoten.

Die TTN-Ansicht zeigt den Join-Status und erlaubt Text-Uplinks bis 50 Zeichen.
Die Firmware sendet diese als unbestätigte LoRaWAN-Uplinks auf FPort 1 und
zeigt empfangene Downlinks an. Erst `TTN|JOINED` bestätigt, dass der Knoten dem
TTN-Netz beigetreten ist.

## Protokoll

Im Blackout-Modus sendet die App eine Zeile `CHAT|Text`. Der ESP bestätigt lokal mit
`TX|id|counter|Text` und meldet LoRa-Empfang als
`RX|sender|counter|rssi|snr|Text`. Pipes und Backslashes im Text werden von
der Firmware escaped, damit das Protokoll eindeutig bleibt.

Fuer Positionen sendet die Firmware:

- `LOC|nodeId|lat|lon|accuracy|timestamp`

