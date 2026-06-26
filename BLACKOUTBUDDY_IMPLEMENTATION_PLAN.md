# BlackoutBuddy – Implementation Plan

## Ziel

Aus dem bestehenden LoRa-basierten Projekt soll ein zweigeteilter Aufbau werden:

1. Blackout-Modus
   - Funktioniert ohne Internet, ohne zentrale Infrastruktur.
   - Nutzt ein lokales Mesh-System zwischen Geräten.
   - Fokus auf Kommunikation in Notfällen und Ausfällen.

2. Connectivity Mode (Non-Blackout-Modus)
   - Funktioniert im normalen Betrieb mit Server, Website und möglicher Internet-/Netzwerkunterstützung.
   - Kann als Standardmodus für Alltagsszenarien dienen.
   - Sollte als klar getrennte Betriebsart neben dem Blackout-Modus existieren.

---

## Aktueller Stand

Das Projekt besitzt bereits:

- eine LoRa-basierte Kommunikationsgrundlage,
- ein ESP32/LoRa-Setup,
- einen Server und eine Weboberfläche,
- einen Basis-Chat-Mechanismus.

Fehlt aktuell vor allem:

- ein echtes Mesh-System im Blackout-Bereich,
- eine klare Trennung zwischen Blackout- und Non-Blackout-Funktionalität,
- ein gemeinsames Daten- und Nachrichtenmodell für beide Modi.

---

## Zukunftsarchitektur

### 1. Blackout-Modus

Der Blackout-Modus soll als primärer Notfallmodus aufgebaut werden.

#### Was implementiert werden muss

- Mesh-Topologie zwischen mehreren Geräten
  - Geräte erkennen sich gegenseitig.
  - Nachrichten können über mehrere Knoten weitergereicht werden.
- Routing-Logik
  - Zieladresse erkennen.
  - Wegfindung zwischen Geräten.
  - Fallback, wenn ein Knoten ausfällt.
- Nachrichten-Weiterleitung
  - Pakete sollen nicht nur direkt, sondern auch relayed übertragen werden.
- Geräte-Identifikation
  - eindeutige IDs pro Gerät.
  - optionaler Gerätename und Rollenmodell.
- Zustandsverwaltung
  - welche Geräte sind online?
  - welche Pakete wurden bereits weitergeleitet?
- Robustheit
  - Wiederholungslogik bei Verlust.
  - Timeout- und Retry-Mechanik.
  - Schutz vor Schleifen im Mesh.

#### Zielzustand

Ein Gerät kann im Blackout-Modus Nachrichten an andere Geräte senden, selbst wenn kein Server, kein Internet und kein zentraler Knoten verfügbar ist.

---

### 2. Non-Blackout-Modus

Der Non-Blackout-Modus soll den normalen Betrieb abbilden.

#### Was implementiert werden muss

- Server-basierte Kommunikation
  - Weboberfläche bleibt der zentrale Einstiegspunkt.
  - Nachrichten laufen über den Server, wenn die Infrastruktur verfügbar ist.
- Modus-Auswahl im Frontend
  - Nutzer kann zwischen Blackout- und Non-Blackout-Modus wählen.
- Fallback-Strategie
  - Wenn der normale Serverweg nicht verfügbar ist, soll auf LoRa bzw. Mesh zurückgefallen werden.
- Zustandsanzeige
  - sichtbar machen, in welchem Modus gerade gearbeitet wird.

#### Zielzustand

Im Non-Blackout-Modus kann das System wie ein normales Kommunikationssystem arbeiten, während im Blackout-Modus die Notfallkommunikation über das Mesh-System läuft.

---

## Gemeinsame Basis für beide Modi

Damit beide Modi sauber funktionieren, sollten gewisse Komponenten zentralisiert werden.

### 1. Nachrichtenmodell

Ein gemeinsames Format für Nachrichten ist erforderlich:

- Absender
- Ziel
- Typ der Nachricht
- Inhalt
- Timestamp
- Priorität
- Modus-Info

### 2. Konfigurationsmodell

Es sollte eine zentrale Konfiguration geben für:

- Geräte-ID
- Gerätename
- Modus
- Frequenz / LoRa-Parameter
- Netzwerk-/Mesh-Einstellungen
- Server-Endpoint

### 3. UI/UX

Die Benutzeroberfläche sollte:

- klar zwischen den beiden Modi unterscheiden,
- Statusinformationen anzeigen,
- einfache Umschaltung ermöglichen,
- Fehlermeldungen verständlich machen.

---

## Arbeiten in Prioritäten

### Phase 1 – Basis vorbereiten

- Projektziel und Moduskonzept festlegen
- bestehende Firmware und Serverstruktur sauber dokumentieren
- ein gemeinsames Nachrichtenformat definieren
- Geräte-IDs und Rollenmodell festlegen

### Phase 2 – Blackout-Mesh aufbauen

- Geräteerkennung einbauen
- einfache Broadcast-/Unicast-Nachrichten implementieren
- Weiterleitung zwischen Geräten ermöglichen
- Wiederholungslogik und Deduplizierung einbauen

### Phase 3 – Modus-Handling einführen

- Blackout- und Non-Blackout-Modus getrennt abbilden
- UI für Moduswechsel ergänzen
- Server- und LoRa-Pfade sauber trennen

### Phase 4 – Robustheit und UX verbessern

- Fehlerszenarien absichern
- Lade- und Batterie-Handling berücksichtigen
- bessere Statusanzeigen und Logging einbauen
- Testfälle für mehrere Geräte definieren

---

## Technische Bausteine

### Firmware / ESP32

- LoRa-Transport erweitern
- Mesh-Logik ergänzen
- Modus-Handling einbauen
- Paket-Parsing und Routing verbessern

### Server

- Nachrichten-Weiterleitung zwischen Weboberfläche und Geräten sauber strukturieren
- Status- und Modusverwaltung ergänzen
- optional: zentrale Sicht auf aktive Knoten

### Website

- Modus-Auswahl einbauen
- Blackout-/Non-Blackout-Ansicht getrennt gestalten
- Statusleiste für Verbindung, Modus und Geräteverfügbarkeit

---

## Definition of Done

Das Projekt gilt als sinnvoll umgesetzt, wenn:

- Blackout-Kommunikation ohne Server möglich ist,
- Non-Blackout-Kommunikation mit Server/Website funktioniert,
- beide Modi sauber auswählbar sind,
- die Geräte in einem Mesh-Szenario zuverlässig Nachrichten weiterleiten,
- die Benutzeroberfläche den aktuellen Modus klar zeigt.

---

## Empfohlene Reihenfolge

1. Mesh-System für den Blackout-Modus bauen
2. Gemeinsames Nachrichtenmodell definieren
3. Moduswechsel und getrennte Logik einführen
4. Website und Server an die neue Architektur anpassen
5. Tests mit mehreren Geräten durchführen

---

## Konkrete Todos

1. Modusstruktur festlegen
   - Blackout-Modus und Connectivity Mode sauber voneinander trennen.

2. Gemeinsames Nachrichtenformat definieren
   - Sender, Ziel, Inhalt, Typ, Timestamp und Priorität festlegen.

3. Blackout-Mesh bauen
   - Geräte erkennen.
   - Nachrichten weiterleiten.
   - Wiederholungen und Schutz vor Schleifen einbauen.

4. Connectivity-Logik ergänzen
   - Server-/Netzwerkweg für normalen Betrieb einrichten.
   - Fallback auf LoRa/Mesh definieren.

5. UI/UX anpassen
   - Modus-Auswahl einbauen.
   - Statusanzeige für aktuellen Betriebszustand ergänzen.

6. Firmware und Server trennen
   - Blackout- und Connectivity-Logik klar strukturieren.

7. Tests mit mehreren Geräten durchführen
   - Mesh-Funktion prüfen.
   - Moduswechsel testen.

---

## Kurzfassung

Die nächste große Aufgabe ist nicht nur „mehr Funk“, sondern die Umstellung auf ein echtes Zwei-Modus-System:

- Blackout = dezentral, robust, mesh-basiert
- Connectivity Mode = zentral, serverbasiert, normaler Betrieb
