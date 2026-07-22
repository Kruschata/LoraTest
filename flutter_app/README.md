# BlackoutBuddy Flutter App

Android-App für den direkten Chat über Bluetooth Classic (SPP) mit einem
LilyGO T-Beam/ESP32. Der ESP leitet die Nachrichten danach per LoRa an andere
Knoten weiter. Im Bluetooth-Modus wird kein Node-Server benötigt.

## Starten

Voraussetzungen: Flutter SDK mit Android SDK und ein echtes Android-Gerät.
`flutter_bluetooth_serial` unterstützt kein iOS. Koppel den ESP vorher in den
Android-Bluetooth-Einstellungen; sein Name lautet `LoRaChat-*`. Der PIN ist
`1234`; nach einem Firmware-Update den bisherigen Eintrag bei Bedarf entfernen
und erneut koppeln.

```powershell
flutter pub get
# Dieses Repository enthält keine generierte Android-Plattform:
flutter create --platforms=android .
flutter run
```

Erlaube beim ersten Start die Bluetooth-Berechtigungen. Die nötigen
Berechtigungen für Android 12+ sind bereits in
`android/app/src/main/AndroidManifest.xml` hinterlegt.

```xml
<uses-permission android:name="android.permission.BLUETOOTH_SCAN" />
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT" />
<uses-permission android:name="android.permission.BLUETOOTH" android:maxSdkVersion="30" />
<uses-permission android:name="android.permission.BLUETOOTH_ADMIN" android:maxSdkVersion="30" />
```

Die App zeigt gekoppelte `LoRaChat-*`-Geräte, sendet `CHAT|<Text>` per
Bluetooth-Serial an den ESP und verarbeitet dessen `TX`-, `RX`- und
`STATUS`-Zeilen vollständig, auch wenn sie in mehreren Bluetooth-Paketen
ankommen.
