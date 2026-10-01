# BlackoutBuddy

Diese Flutter-App ist für das aktuelle Projekt auf Android und Web optimiert. Sie dient als Frontend für eine lokale Kommunikationslösung mit Bluetooth-basierter Geräteverbindung und LoRa-gestützter Knotenkommunikation.

## Aktueller Projektstatus

- Unterstützte Plattformen: Android, Web

## Zielsetzung

- Geräte im lokalen Netzwerk bzw. über gekoppelte Bluetooth-Geräte erkennen
- Nachrichten an LoRa-fähige Knoten senden und empfangen
- Kommunikationslogik im lokalen Umfeld ohne zentrale Server-Infrastruktur betreiben

## Voraussetzungen

- Flutter SDK 3.x
- Android Studio mit Android SDK
- Android-Gerät mit Bluetooth Classic-Unterstützung

## Setup

```powershell
cd app
flutter pub get
```

Wenn die Plattformen noch nicht existieren oder nach einer Neuinitialisierung eingerichtet werden müssen:

```powershell
flutter create --platforms=android,web .
```

## App starten

Android:

```powershell
flutter run -d android
```

Web:

```powershell
flutter run -d chrome
```

## Build

Android APK:

```powershell
flutter build apk
```

Web Build:

```powershell
flutter build web
```

## Bluetooth/Hardware-Umgebung

Die App erwartet eine direkte Bluetooth-Verbindung zu einem kompatiblen Gerät, z. B. einem ESP32/LilyGO-basierenden Knoten im Format `LoRaChat-*`. Bei der Einrichtung kann ein PIN wie `1234` erforderlich sein. Die benötigten Android-Berechtigungen für Bluetooth sind in der Android-Manifest-Datei hinterlegt.

## Kommunikationspfad

1. App -> Gerät per Bluetooth Classic (SPP)
2. Gerät -> andere Knoten per LoRa
3. lokale Kommunikation ohne zentralen Server

Damit können Nachrichten im dezentralen Netzwerk übertragen werden, solange die beteiligten Geräte kompatibel konfiguriert sind.

## Hinweis

Dieses README beschreibt den aktuellen, vereinfachten Projektstand. 
