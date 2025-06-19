#include <SPI.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <avr/wdt.h> // Watchdog timer for stability

#define ETHERNET_SHIELD_MAC {0xC0, 0x26, 0x1B, 0x5A, 0x70, 0x26}

const int gasPin = A0;
const int buzz = 3;

byte ethernetMACAddress[] = ETHERNET_SHIELD_MAC;
EthernetClient client;

// ThingSpeak Settings
char thingSpeakAddress[] = "api.thingspeak.com";
String writeAPIKey = "KE099C98QMLUZR0G";
const int updateThingSpeakInterval = 15000; // 15 seconds (API limit)

long lastConnectionTime = 0;
boolean lastConnected = false;
int failedCounter = 0;

void setup() {
    pinMode(buzz, OUTPUT);
    Serial.begin(9600);
    startEthernet();
    wdt_enable(WDTO_8S); // Enable watchdog timer (resets Arduino if frozen)
}

void loop() {
    wdt_reset(); // Reset watchdog timer to prevent unwanted resets
    updateCloud();
    int gasValue = analogRead(gasPin);
    Serial.println("Gas Sensor Value: " + String(gasValue));
    if (gasValue > 250) {
        digitalWrite(buzz, HIGH);
        sendEmail();
    } else {
        digitalWrite(buzz, LOW);
    }
    delay(1000);
}

void updateCloud() {
    String gasData = "field1=" + String(analogRead(A0));
    if (!client.connected() && (millis() - lastConnectionTime > updateThingSpeakInterval)) {
        updateThingSpeak(gasData);
    }
    lastConnected = client.connected();
}

void updateThingSpeak(String tsData) {
    if (client.connect(thingSpeakAddress, 80)) {
        client.print("POST /update HTTP/1.1\n");
        client.print("Host: api.thingspeak.com\n");
        client.print("Connection: close\n");
        client.print("X-THINGSPEAKAPIKEY: " + writeAPIKey + "\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: ");
        client.println(tsData.length());
        client.println();
        client.println(tsData);
        lastConnectionTime = millis();
        Serial.println("Data sent to ThingSpeak");
        failedCounter = 0;
    } else {
        failedCounter++;
        Serial.println("ThingSpeak connection failed (" + String(failedCounter) + ")");
        if (failedCounter > 3) startEthernet();
    }
    client.stop();
}

void startEthernet() {
    client.stop();
    Serial.println("Connecting to network...");
    if (Ethernet.begin(ethernetMACAddress) == 0) {
        Serial.println("DHCP Failed, retrying...");
    } else {
        Serial.println("Connected to network");
    }
    delay(1000);
}

void sendEmail() {
    Serial.println("Sending Email Alert...");
    if (client.connect("smtp.example.com", 587)) { // Replace with SMTP provider
        client.println("EHLO smtp.example.com");
        client.println("AUTH LOGIN");
        client.println("BASE64_ENCODED_USERNAME"); // Replace with actual base64 encoded username
        client.println("BASE64_ENCODED_PASSWORD"); // Replace with actual base64 encoded password
        client.println("MAIL FROM: <you@example.com>");
        client.println("RCPT TO: <recipient@example.com>");
        client.println("DATA");
        client.println("Subject: Gas Alert!\n");
        client.println("Gas levels exceeded safe limits! Take action immediately.\n.");
        client.println("QUIT");
        Serial.println("Email Sent!");
    } else {
        Serial.println("Email sending failed");
    }
    client.stop();
}
