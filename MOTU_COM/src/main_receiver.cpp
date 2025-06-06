#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#ifdef RECEIVER
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define DEVICE_NAME "B"  // Set this ESP32 as "B"

// OLED Display Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define I2C_SDA 2
#define I2C_SCL 3
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Firebase Objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData streamData;
String uid;

void initOLED() {
    Wire.begin(I2C_SDA, I2C_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed");
        for (;;);
    }
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("ESP32C3-OLED Ready!");
    display.display();
}
// Display Message on OLED
void displayMessage(String message) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Motu:");
    display.println(message);
    display.display();
}


// WiFi Initialization
void initWiFi() {
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
    displayMessage("Connected");
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
    config.max_token_generation_retry = 10;  // Increased retries for token renewal

    Firebase.begin(&config, &auth);

    Serial.println("Getting User UID...");
    while (auth.token.uid == "") {
        Serial.print(".");
        delay(1000);
    }
    uid = auth.token.uid.c_str();
    Serial.println("\nUser UID: " + uid);
}

// OLED Initialization


// 📡 Stream Callback Function (Receives Messages from ESP32-A)
void streamCallback(FirebaseStream data) {
    Serial.println("\nNew Message Received from ESP32-A!");
    if (data.dataType() == "string") {
        String receivedMsg = data.stringData();
        Serial.println("Received: " + receivedMsg);
        displayMessage(receivedMsg);  // Show message on OLED
    }
}

// 📡 Stream Timeout Handler
void streamTimeoutCallback(bool timeout) {
    if (timeout) {
        Serial.println(" Firebase Stream Timeout! Restarting Stream...");
        Firebase.RTDB.endStream(&streamData);
        Firebase.RTDB.beginStream(&streamData, "/messages/espB/message");
        Firebase.RTDB.setStreamCallback(&streamData, streamCallback, streamTimeoutCallback);
    }
}


void setup() {
    Serial.begin(115200);
    initOLED();
    initWiFi();
    initFirebase();
    

    // ✅ Start Firebase Streaming (Listening for messages from ESP32-A)
    String path = "/messages/espB/message";  // Listening for messages meant for ESP32-B
    Firebase.RTDB.beginStream(&streamData, path);
    Firebase.RTDB.setStreamCallback(&streamData, streamCallback, streamTimeoutCallback);
}

void loop() {
    // 🔄 Reconnect WiFi if Disconnected
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("⚠️ WiFi Disconnected! Reconnecting...");
        WiFi.disconnect();
        WiFi.reconnect();
        delay(5000);
        return; // Skip loop iteration if not connected
    }

    // 🔄 Check Firebase Stream Status
    if (streamData.streamPath().length() == 0 || !streamData.httpConnected()) {
        
        Serial.println(" Firebase Stream Disconnected! Restarting...");

        // End the current stream (if any)
        Firebase.RTDB.endStream(&streamData);

        // Restart the stream
        Firebase.RTDB.beginStream(&streamData, "/messages/espB/message");
        Firebase.RTDB.setStreamCallback(&streamData, streamCallback, streamTimeoutCallback);
    }

    // 🔄 Handle Firebase Token Expiry (Better Approach)
    if (!Firebase.ready()) {
        Serial.println(" Firebase Token Expired! Re-authenticating...");
        fbdo.clear();
        streamData.clear();
        initFirebase(); // Restart Firebase authentication
    }

    delay(1000); // Avoid unnecessary CPU usage
}



#endif
