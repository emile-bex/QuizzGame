#include <WiFiManager.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <Arduino.h>

const int buttonPin = 2;
const int buzzerPin =  0;

int buttonState = 0;

bool buttonPushed = false;

int buttonClickCount = 0;

const String RESET = "RESET";
const String HANDSHAKE_1 = "HANDSHAKE_1";
const String HANDSHAKE_2 = "HANDSHAKE_2";
const String HANDSHAKE_3 = "HANDSHAKE_3";
const String HANDSHAKE_4 = "HANDSHAKE_4";

unsigned long long ntpOffset = 0;

const char* websocketURL = "ws://192.168.1.62:3000";

using namespace websockets;

WebsocketsClient client;

WiFiManager wm;

void sendMessage(const StaticJsonDocument<64>& json) {
    String output = "";

    serializeJson(json, output);

    client.send(output);

    Serial.println("Sent : " + output);

}

void onMessageCallback(const WebsocketsMessage& message) {
    Serial.println("Got Message : " + message.data());

    StaticJsonDocument<64> event;

    deserializeJson(event, message.data());

    String eventType = event["type"];

    if (eventType == HANDSHAKE_2) {
        StaticJsonDocument<64> handshakeEvent;

        handshakeEvent["type"] = HANDSHAKE_3;
        handshakeEvent["t1"] = event["t1"];
        handshakeEvent["t2"] = millis();

        sendMessage(handshakeEvent);
    }

    if (eventType == HANDSHAKE_4) {
        StaticJsonDocument<64> handshakeEvent;

        unsigned long long t1 = event["t1"];
        unsigned long long t2 = event["t2"];
        unsigned long long t3 = event["t3"];
        unsigned long long t4 = millis();

        ntpOffset = ((t2 - t1) + (t3 - t4)) / 2;

        Serial.println("Offset : " + String(ntpOffset));
    }

    Serial.println("Got Message : " + message.data());
}

void onEventsCallback(WebsocketsEvent event, const String& data) {
    if (event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Connnection Opened");
    } else if (event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Connnection Closed");
    } else if (event == WebsocketsEvent::GotPing) {
        Serial.println("Got a Ping!");
    } else if (event == WebsocketsEvent::GotPong) {
        Serial.println("Got a Pong!");
    }
}

void setup() {
    pinMode(buzzerPin, OUTPUT);
    pinMode(buttonPin, INPUT_PULLUP);

    Serial.begin(115200);

    WiFi.mode(WIFI_STA);

    bool res;

    res = wm.autoConnect("AutoConnectAP", "password");

    if (!res) {
        Serial.println("Failed to connect");
    }
    else {
        Serial.println("Connected");

        // Setup Callbacks
        client.onMessage(onMessageCallback);
        client.onEvent(onEventsCallback);

        // Connect to server
        client.connect(websocketURL);

        client.ping();

        StaticJsonDocument<64> handshakeEvent;

        unsigned long handshakeMillis = millis();

        handshakeEvent["type"] = HANDSHAKE_1;

        sendMessage(handshakeEvent);
    }
}

void loop() {
    if (Serial.available()) {
        String receivedString = Serial.readString();
        receivedString.trim();

        Serial.println("I received: " + receivedString);

        if (receivedString == RESET) {
            Serial.println("Resetting WiFi settings");
            wm.resetSettings();
            Serial.println("WiFi settings reset");
            EspClass::eraseConfig();
            delay(2000);
            EspClass::reset();
        }
    }

    buttonState = digitalRead(buttonPin);

    if (buttonState == LOW) {
        if (!buttonPushed) {
            buttonPushed = true;
            buttonClickCount += 1;

            StaticJsonDocument<64> clickEvent;

            clickEvent["id"] = "red";
            clickEvent["count"] = buttonClickCount;
            clickEvent["time"] = millis() + ntpOffset;

            sendMessage(clickEvent);
        }
        tone(buzzerPin, 50);
    }
    else {
        buttonPushed = false;
        noTone(buzzerPin);
    }

    client.poll();
}