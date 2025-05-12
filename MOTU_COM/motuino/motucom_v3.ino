
//for esp32 dev boards sda-d21 scl d22
//esp32c3 sda d4 and sck d5

//for esp32 dev boards sda-d21 scl d22
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "bitmap.h"
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <WiFiManager.h>

// Define the device name as ESP32-B
#define DEVICE_NAME "B"

// OLED Display Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Button Pin Definition
#define BUTTON_PIN 9  // Adjust as per your button pin

// Firebase Objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData streamData;
String uid;


// Slideshow state
const uint8_t* slideshowImages[] = { img1, img2, img3, img4, img5 };
int currentImageIndex = 0;
bool displayImages = false;
String receivedMsg = "";

void initOLED() {
    Wire.begin();
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
    display.println(" San:");
    display.println(message);
    display.display();
}

// WiFi Initialization
void initWiFi() {
    WiFiManager wm;
    wm.setTimeout(270); //turns ooff after 3 mins
    if (!wm.autoConnect("PESP")) {
        Serial.println(" Failed to connect or timeout");
        // Optionally reset or fallback
        ESP.restart();
    }
    Serial.println(WiFi.localIP());
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Wifi"); 
    display.println("Connected");
    display.display();
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

// 📡 Stream Callback Function (Receives Messages from ESP32-A)
void streamCallback(FirebaseStream data) {
    Serial.println("\nNew Message Received from ESP32-A!");
    if (data.dataType() == "string") {
        receivedMsg  = data.stringData();
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

// Handle Button Press for Mode Toggle
void toggleDisplayMode() {
    static unsigned long lastPressTime = 0;
    unsigned long currentTime = millis();
    
    // Button pressed logic
    if (digitalRead(BUTTON_PIN) == LOW && currentTime - lastPressTime > 200) { // Debounce time
        displayImages = !displayImages;  // Toggle between modes
        lastPressTime = currentTime;
        if (displayImages) {
            Serial.println("Entering Slideshow Mode");
             int frame = 0; int counter = 0;
            while(true){
                display.clearDisplay();
                display.drawBitmap(32, 0, picframes[frame], FRAME_WIDTH, FRAME_HEIGHT, 1);
                display.display();
                delay(FRAME_DELAY);
                frame++; 
                if(frame==P_FRAME_COUNT){frame=0;counter++;}
                if(counter==2) break;
                
            }
        } 
        
        else {
            Serial.println("Entering Firebase Mode");
            int frame = 0; int counter = 0;
            while(true){
                display.clearDisplay();
                display.drawBitmap(32, 0, txframes[frame], FRAME_WIDTH, FRAME_HEIGHT, 1);
                display.display();
                delay(FRAME_DELAY);
                frame++; 
                if(frame==T_FRAME_COUNT){frame=0;counter++;}
                if(counter==2) break;
                
            }
            displayMessage(receivedMsg);
        }
    }
}

// Display Image for Slideshow
void displaySlideshowImage() {
    display.clearDisplay();
    display.drawBitmap(0, 0, slideshowImages[currentImageIndex], SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    display.display();
}

// Cycle through slideshow images
void nextImage() {
    currentImageIndex = (currentImageIndex + 1) % (sizeof(slideshowImages) / sizeof(slideshowImages[0]));
    displaySlideshowImage();
}
void on(){
    int frame = 0; int counter = 0;
    while(true){
        display.clearDisplay();
        display.drawBitmap(32, 0, onframes[frame], FRAME_WIDTH, FRAME_HEIGHT, 1);
        display.display();
        delay(FRAME_DELAY);
        frame++; 
        if(frame==O_FRAME_COUNT){frame=0;counter++;}
        if(counter==3) break;
    }
}
void setup() {
    Serial.begin(115200); 
    initOLED();
    on();
    initWiFi();
    initFirebase();

    // Set button pin as input
    pinMode(BUTTON_PIN, INPUT_PULLUP);

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
        Firebase.RTDB.endStream(&streamData);
        Firebase.RTDB.beginStream(&streamData, "/messages/espB/message");
        Firebase.RTDB.setStreamCallback(&streamData, streamCallback, streamTimeoutCallback);
    }

    // 🔄 Handle Firebase Token Expiry
    if (!Firebase.ready()) {
        Serial.println(" Firebase Token Expired! Re-authenticating...");
        fbdo.clear();
        streamData.clear();
        initFirebase(); // Restart Firebase authentication
    }

    // Toggle between Firebase display and slideshow on button press
    toggleDisplayMode();

    // Display images in slideshow mode
    if (displayImages) {
        
        static unsigned long lastPressTime = 0;
        unsigned long currentTime = millis();
        if(currentTime - lastPressTime > 6000){nextImage();lastPressTime=millis();}
    }

    static unsigned long cpuRelaxTime = 0;
    if (millis() - cpuRelaxTime >= 1000) {
        cpuRelaxTime = millis();
        // This runs every 1 second
    }

}
