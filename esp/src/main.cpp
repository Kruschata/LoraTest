#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Passen Sie diese Pins an Ihre LoRa-Hardware an.
// Häufige ESP32-Verkabelung: SS=18, RST=14, DIO0=26
const int LORA_SS = 18;
const int LORA_RST = 14;
const int LORA_DIO0 = 26;
const long LORA_FREQUENCY = 868E6;

// Eindeutige Geräte-ID (ändern für jedes Gerät)
#define DEVICE_ID 2

// OLED-Konfiguration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void showOnDisplay(const String &line1, const String &line2) {
    String l1 = line1;
    String l2 = line2;
    if (l1.length() > 20) l1 = l1.substring(0, 20) + "...";
    if (l2.length() > 30) l2 = l2.substring(0, 30) + "...";

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(l1);
    display.setCursor(0, 18);
    display.println(l2);
    display.display();
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    // I2C Pins für viele Lilygo Boards: SDA=21, SCL=22
    Wire.begin(21, 22);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 Init fehlgeschlagen");
    } else {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,0);
        display.println("OLED initialisiert");
        display.println(String("Device ID: ") + String(DEVICE_ID));
        display.display();
    }

    SPI.begin(5, 19, 27, LORA_SS);
    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

    if (!LoRa.begin(LORA_FREQUENCY)) {
        Serial.println("LoRa-Initialisierung fehlgeschlagen!");
        while (true) {
            delay(1000);
        }
    }

    Serial.println("LoRa bereit");
}

void loop()
{
    if (Serial.available()) {
        String message = Serial.readStringUntil('\n');
        message.trim();

        if (message.length() > 0) {
            // Wenn die Website / Server ein Ziel angibt: "TO:<id>|Text"
            // wir senden immer mit FROM:<DEVICE_ID>|...
            String payload;
            if (message.startsWith("TO:")) {
                payload = "FROM:" + String(DEVICE_ID) + "|" + message;
            } else {
                payload = "FROM:" + String(DEVICE_ID) + "|MSG:" + message;
            }

            LoRa.beginPacket();
            LoRa.print(payload);
            LoRa.endPacket();

            Serial.print("Sende LoRa: ");
            Serial.println(payload);
            showOnDisplay("Sende:", payload);
        }
    }

    int packetSize = LoRa.parsePacket();
    if (packetSize > 0) {
        String incoming = "";
        while (LoRa.available()) {
            incoming += (char)LoRa.read();
        }

        incoming.trim();
        if (incoming.length() > 0) {
            // Wir geben Roh-Nachricht über Serial aus, damit Server/Client sie sehen
            Serial.print(incoming);
            Serial.println();
            showOnDisplay("Empfangen:", incoming);
        }
    }
}
