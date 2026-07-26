# BlackoutBuddy - Implementation Plan (Aktueller Scope)

## Ziel

Ziel ist ein robustes, serverloses Zwei-Geraete-Setup:

1. Smartphone <-> ESP32/T-Beam ueber Bluetooth Classic (SPP)
2. ESP32/T-Beam <-> ESP32/T-Beam ausschliesslich ueber LoRa

Es gibt keinen WebSocket-, HTTP- oder Cloud-Pfad im aktuellen Scope.

## Aktueller Ist-Stand

- Flutter-App bietet nur Bluetooth-Verbindung zum gekoppelten LoRaChat-Knoten.
- ESP-Firmware verarbeitet `CHAT|...`-Kommandos und sendet/empfaengt LoRa-Payloads.
- LoRa-Status und Nachrichten werden als `TX|...`, `RX|...`, `STATUS|...` an die App zurueckgegeben.
- Escaping fuer `|` und `\\` ist in der Firmware vorhanden.

## Kommunikationsfluss

1. Nutzer schreibt Nachricht in der App.
2. App sendet `CHAT|Text` per Bluetooth an den lokalen ESP.
3. ESP sendet LoRa-Paket an den anderen ESP.
4. Empfangender ESP meldet `RX|...` lokal (USB/Bluetooth).

## Was fuer 2 Geraete verpflichtend ist

- Eindeutige `DEVICE_ID` pro Knoten in `esp/src/main.cpp`.
- Eindeutiger Bluetooth-Name pro Knoten (`LoRaChat-1`, `LoRaChat-2`, ...).
- Identische LoRa-Parameter auf allen Knoten:
  - Frequenz
  - Sync Word
  - Spreading Factor
  - Bandwidth
  - Coding Rate
- Knoten vorab in Android koppeln (PIN `1234`).

## Bekannte Grenzen

- Kein Multi-Hop-Relay: Empfangene LoRa-Nachrichten werden derzeit nicht weitergeleitet.
- Kein Routing/Zieladressierung fuer groessere Mesh-Topologien.
- Kein Store-and-Forward und keine Retry-Strategie auf Routing-Ebene.

## Naechste Ausbaustufen (optional)

1. Deduplizierung ueber Nachricht-ID (Sender + Counter).
2. TTL/Hop-Count in Payload aufnehmen.
3. Optionales Relay fuer Multi-Hop-Netze aktivieren.
4. Zieladressierung (Unicast/Broadcast) einfuehren.
5. Einfache Delivery-ACKs fuer bessere Zuverlaessigkeit.

## Definition of Done (aktueller Scope)

- Zwei T-Beams sind unterschiedlich konfiguriert und erfolgreich geflasht.
- Beide Knoten sind in Android als Bluetooth-Geraete sichtbar und koppelbar.
- Nachrichten von Geraet A erscheinen auf Geraet B ueber LoRa.
- Nachrichten von Geraet B erscheinen auf Geraet A ueber LoRa.
- Keine Server-/WebSocket-Komponente wird fuer den Betrieb benoetigt.
