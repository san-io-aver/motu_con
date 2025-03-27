#include <Arduino.h>
#include <WiFi.h>
#ifdef SENDER 
#include <Firebase_ESP_Client.h>
#include "config.h"
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// 🔹 Set "A" for one ESP32, "B" for the other
#define DEVICE_NAME "A"  // Change to "B" on the other ESP32C3

// Firebase Objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData streamData;

String uid;

// WiFi Initialization
void initWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
}

// Firebase Initialization
void initFirebase() {
    config.api_key = API_KEY;
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
    config.database_url = DATABASE_URL;

    Firebase.reconnectWiFi(true);
    fbdo.setResponseSize(4096);
    config.token_status_callback = tokenStatusCallback;
    config.max_token_generation_retry = 5;

    Firebase.begin(&config, &auth);

    Serial.println("Getting User UID...");
    while (auth.token.uid == "") {
        Serial.print(".");
        delay(1000);
    }
    uid = auth.token.uid.c_str();
    Serial.println("\nUser UID: " + uid);
}

// 📡 Stream Callback Function (Receives Messages from Web)
void streamCallback(FirebaseStream data) {
    Serial.println("\n New Message Received from ESP32-B");
    if (data.dataType() == "string") {
        Serial.println("Received: " + data.stringData());
    }
}

// 📡 Stream Timeout
void streamTimeoutCallback(bool timeout) {
    if (timeout) {
        Serial.println(" Firebase Stream Timeout, Reconnecting...");
    }
}

void setup() {
    Serial.begin(115200);
    initWiFi();
    initFirebase();

    // ✅ Start Firebase Streaming (Listen for messages for this ESP32)
    String path = "/messages/espA";
    Firebase.RTDB.beginStream(&streamData, path);
    Firebase.RTDB.setStreamCallback(&streamData, streamCallback, streamTimeoutCallback);
}

void loop() {
    delay(1000); // Just keeping it running
}
#endif