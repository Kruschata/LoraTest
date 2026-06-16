#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

// Passen Sie diese Pins an Ihre LoRa-Hardware an.
// Häufige ESP32-Verkabelung: SS=18, RST=14, DIO0=26
const int LORA_SS = 18;
const int LORA_RST = 14;
const int LORA_DIO0 = 26;
const long LORA_FREQUENCY = 868E6;

void setup()
{
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
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
            LoRa.beginPacket();
            LoRa.print(message);
            LoRa.endPacket();

            Serial.print("Sende LoRa: ");
            Serial.println(message);
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
            Serial.print("LoRa empfangen: ");
            Serial.println(incoming);
        }
    }
}
