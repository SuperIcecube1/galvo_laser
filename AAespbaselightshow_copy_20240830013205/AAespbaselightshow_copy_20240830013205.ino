#include <WiFi.h>
#include <HTTPClient.h>
#include <FastLED.h>
#include <ArduinoJson.h>

#define NUM_LEDS 60
#define DATA_PIN1 16
#define DATA_PIN2 17
#define DATA_PIN3 18
#define DATA_PIN4 19

// External functions declared for scene and effect handling
extern void RaveEffect();
extern void ClubEffect();
extern void DiscoEffect();
extern void Effect1();
extern void Effect2();
extern void Effect3();
extern void Effect4();
extern void Effect5();
extern void Effect6();
extern void Effect7();
extern void Effect8();
extern void Effect9();
extern void Effect10();

CRGB leds1[NUM_LEDS];
CRGB leds2[NUM_LEDS];
CRGB leds3[NUM_LEDS];
CRGB leds4[NUM_LEDS];

const char* ssid = "EmpshottWifi";            // Replace with your WiFi SSID
const char* password = "fA7beXAJKLJpdx";     // Replace with your WiFi password
const char* serverUrl = "http://192.168.1.131:8080/status";  // URL of the server providing BPM and status

int bpm = 120;
int energy_level = 0;
int is_beat_drop = 0;
int mode = 0;  // Combined variable for both scenes and effects
CRGB colors[4] = {CRGB::White, CRGB::White, CRGB::White, CRGB::White}; // RGB colors for each LED strip

// Function to handle HTTP communication on core 0
void handleHTTP(void * parameter) {
    unsigned long previousMillis = 0;
    const long interval = 100; // Set the interval to 100ms for 10 requests per second

    while (true) {
        unsigned long currentMillis = millis();

        if (WiFi.status() == WL_CONNECTED) {
            if (currentMillis - previousMillis >= interval) {
                previousMillis = currentMillis;

                HTTPClient http;
                http.begin(serverUrl);
                int httpResponseCode = http.GET();

                if (httpResponseCode == 200) {
                    String payload = http.getString();
                    parseServerResponse(payload);
                } else {
                    Serial.print("Error in HTTP request: ");
                    Serial.println(httpResponseCode);
                }

                http.end();
            }
        } else {
            Serial.println("WiFi Disconnected. Attempting to reconnect...");
            WiFi.reconnect();
            delay(500); // Add a short delay to avoid flooding with reconnect attempts
        }

        delay(1);  // Small delay to prevent task starvation
    }
}

// Function to handle LED effects on core 1
void handleLEDEffects(void * parameter) {
    while (true) {
        switch (mode) {
            case 1:
                ClubEffect();  // Call the Club Effect
                break;
            case 2:
                RaveEffect();  // Call the Rave Effect
                break;
            case 3:
                DiscoEffect();  // Call the Disco Effect
                break;
            case 4:
                Effect1();  // Call Effect 1
                break;
            case 5:
                Effect2();  // Call Effect 2
                break;
            case 6:
                Effect3();  // Call Effect 3
                break;
            case 7:
                Effect4();  // Call Effect 4
                break;
            case 8:
                Effect5();  // Call Effect 5
                break;
            case 9:
                Effect6();  // Call Effect 6
                break;
            case 10:
                Effect7();  // Call Effect 7
                break;
            case 11:
                Effect8();  // Call Effect 8
                break;
            case 12:
                Effect9();  // Call Effect 9
                break;
            case 13:
                Effect10();  // Call Effect 10
                break;
            default:
                FastLED.clear();
                FastLED.show();
                break;
        }

        delay(10);  // Small delay to prevent potential overload
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi...");
    }

    Serial.println("Connected to WiFi");

    // Print the IP address of the ESP32
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    FastLED.addLeds<WS2812, DATA_PIN1, GRB>(leds1, NUM_LEDS);
    FastLED.addLeds<WS2812, DATA_PIN2, GRB>(leds2, NUM_LEDS);
    FastLED.addLeds<WS2812, DATA_PIN3, GRB>(leds3, NUM_LEDS);
    FastLED.addLeds<WS2812, DATA_PIN4, GRB>(leds4, NUM_LEDS);

    FastLED.clear();
    FastLED.show();

    // Create a task to run handleHTTP on core 0
    xTaskCreatePinnedToCore(
        handleHTTP,   // Function to be called
        "HandleHTTP", // Name of the task
        10000,        // Stack size (in words)
        NULL,         // Task input parameter
        2,            // Increased Priority of the task
        NULL,         // Task handle
        0             // Core to run the task on (core 0)
    );

    // Create a task to handle LED effects on core 1
    xTaskCreatePinnedToCore(
        handleLEDEffects,   // Function to be called
        "HandleLEDEffects", // Name of the task
        10000,              // Stack size (in words)
        NULL,               // Task input parameter
        1,                  // Priority of the task
        NULL,               // Task handle
        1                   // Core to run the task on (core 1)
    );
}

float volume_level = 0.0;  // Store the volume level globally

void parseServerResponse(String payload) {
    // Use ArduinoJson to parse the JSON payload
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
        bpm = doc["spotify_bpm"] | 120;
        energy_level = doc["energy_level"] | 0;
        is_beat_drop = doc["is_beat_drop"] == 1;  // Convert 1/0 to true/false
        mode = doc["current_mode"] | 0;
        volume_level = doc["volume_level"] | 0.0;  // Fetch volume level from the server

        // Parse RGB colors for each strip
        JsonArray colorsArray = doc["colors"].as<JsonArray>();
        for (int i = 0; i < 4; i++) {
            const char* colorStr = colorsArray[i];
            colors[i] = CRGB(strtol(&colorStr[1], NULL, 16)); // Convert hex string to CRGB
        }

        // Adjust BPM if less than 120
        if (bpm < 120) {
            bpm *= 2;
        }
    } else {
        Serial.print("JSON deserialization failed: ");
        Serial.println(error.c_str());
    }
}


void loop() {
    // The main loop can be empty because we're using tasks for all ongoing processes
}
