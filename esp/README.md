# BlackoutBuddy ESP Firmware

Firmware für einen LilyGO T-Beam mit ESP32 und SX1276/SX1278. Das Handy
verbindet sich per Bluetooth Classic (SPP) mit einem Knoten; die Knoten
kommunizieren untereinander über LoRa.

## Vor dem Flashen

In `src/main.cpp` jedem Knoten eine eindeutige ID und einen eindeutigen
Bluetooth-Namen geben:

```cpp
static const uint8_t DEVICE_ID = 1;
static const char *BT_NAME = "LoRaChat-1";
```

Alle Knoten müssen dieselbe LoRa-Frequenz und dieselben LoRa-Parameter
verwenden. `868E6` ist passend für EU-868, `915E6` für US-915 und `433E6` für
433-MHz-Module. Vor dem Senden immer eine passende Antenne anschließen.

Die verwendeten Pins sind SCK 5, MISO 19, MOSI 27, NSS 18, RST 23 und DIO0 26.

## Build und Upload

```powershell
cd esp
pio run -e t-beam -t upload
pio device monitor -e t-beam
```

Danach den Knoten in Android als `LoRaChat-*` koppeln und ihn in der App im
Bluetooth-Modus auswählen.

## Protokoll

Die App sendet eine Zeile `CHAT|Text`. Der ESP bestätigt lokal mit
`TX|id|counter|Text` und meldet LoRa-Empfang als
`RX|sender|counter|rssi|snr|Text`. Pipes und Backslashes im Text werden von
der Firmware escaped, damit das Protokoll eindeutig bleibt.
