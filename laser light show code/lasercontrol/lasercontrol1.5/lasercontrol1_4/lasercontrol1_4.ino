#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <IRremote.hpp> // Ensure IRremote 4.x is installed
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "effects.h"    // Include the effects header

// OLED display parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Define OLED display regions
#define TOP_HEIGHT 16
#define BOTTOM_HEIGHT (SCREEN_HEIGHT - TOP_HEIGHT)
#define TOP_START_Y 0
#define BOTTOM_START_Y TOP_HEIGHT

// Define pins
const int SDA_PIN = 22;
const int SCL_PIN = 23;
const int RED_LASER_PIN = 21;
const int GREEN_LASER_PIN = 19;
const int BLUE_LASER_PIN = 18;
const int X_GALVO_PIN = 25; // GPIO25 (DAC1)
const int Y_GALVO_PIN = 26; // GPIO26 (DAC2)
const int IR_RECEIVER_PIN = 12; // Pin for the IR receiver

// EEPROM addresses
#define EEPROM_SSID_ADDR 0
#define EEPROM_PASSWORD_ADDR 64
#define EEPROM_PYTHON_IP_ADDR 128
#define EEPROM_ENERGY_EFFECTS_ADDR 192
#define EEPROM_ENERGY_LEVEL3_EFFECTS_ADDR 208
#define EEPROM_REMOTE_EFFECTS_ADDR 240 // Adjusted address
#define EEPROM_BEAT_DROP_EFFECTS_ADDR 320 // Adjusted address
#define EEPROM_EFFECT_PLAYTIME_ADDR 400   // New address for effect playtime

// Wi-Fi network credentials
char storedSSID[32] = "";
char storedPassword[64] = "";
char storedPythonServerIP[32] = "";

// Energy level effects and remote effects
int energyLevelEffects[3] = {0, 0, 0}; // Energy levels 0 to 2
int energyLevel3Effects[8] = {0};      // Energy level 3 effects
int remoteEffects[20] = {0};           // Remote effects

// Beat drop effects (array of 8 effects)
int beatDropEffects[8] = {0}; // Initialize to 0
int beatDropIndex = 0;        // Index to cycle through beat drop effects

// Energy level 3 index
int energyLevel3Index = 0;    // Index to cycle through energy level 3 effects

// Global playtime value (in seconds)
int effectPlaytime = 3; // Default to 3 seconds

// Web server
WebServer server(80);

// State variables
bool isAPMode = false;
bool remoteMode = false;

// Variables to manage effect execution
volatile int desiredEnergyLevel = -1;
volatile int currentEnergyLevel = -1;

volatile bool beatDropPending = false;
volatile bool beatDropActive = false;

// IR receiver
IRrecv irrecv(IR_RECEIVER_PIN);

// Task handles
TaskHandle_t EffectTaskHandle = NULL;
TaskHandle_t FetchDataTaskHandle = NULL;

// Global laser color variables
uint8_t laserRed = 255;
uint8_t laserGreen = 255;
uint8_t laserBlue = 255;

// Function prototypes
void EffectTask(void *pvParameters);
void FetchDataTask(void *pvParameters);
void startAPMode();
void startMainMode();
void handleRootAP();
void handleConnect();
void handleRootMain();
void handleSetEffects();
void handleSetEnergyLevel3Effects();
void handleSetRemoteEffects();
void handleSetPythonServerIP();
void handleSetBeatDropEffects();
void handleSetEffectPlaytime();
void fetchData();
void handleEffects(int is_beat_drop, int energy_level);
void handleIRRemote();
void handleIRMenu();
void clearEEPROM();
void displayStatus();
void updateIPAddressDisplay();
void updateTopDisplay(String message);
void updateBottomDisplay(String message);
void displayMessage(String message);
void serialPrintln(String message);
void runEffect(int effectNumber);
String translateEncryptionType(uint8_t encryptionType);
void parseHexColor(String hexColor, uint8_t &red, uint8_t &green, uint8_t &blue);
void loadSettings();
void saveSettings();

// Serial print buffer for OLED display
#define SERIAL_PRINT_BUFFER_SIZE 10
String serialPrintBuffer[SERIAL_PRINT_BUFFER_SIZE];
int serialPrintIndex = 0;

// Flag to prevent repeated "Python server IP not set" messages
bool pythonIPNotSetPrinted = false;

// Variables to store current energy level and beat drop status
int currentBeatDrop = 0;

// Define IR Codes based on your remote (update with your remote's codes)
const uint32_t IR_CODE_UP    = 0xB946FF00;
const uint32_t IR_CODE_DOWN  = 0xEA15FF00;
const uint32_t IR_CODE_OK    = 0xBF40FF00;
const uint32_t IR_CODE_BACK  = 0xBB44FF00;
const uint32_t IR_CODE_1     = 0xE916FF00;
const uint32_t IR_CODE_2     = 0xE619FF00;
const uint32_t IR_CODE_3     = 0xF20DFF00;
const uint32_t IR_CODE_4     = 0xF30CFF00;
const uint32_t IR_CODE_5     = 0xE718FF00;
const uint32_t IR_CODE_6     = 0xA15EFF00;
const uint32_t IR_CODE_7     = 0xF708FF00;
const uint32_t IR_CODE_8     = 0xE31CFF00;
const uint32_t IR_CODE_9     = 0xA55AFF00;

// Mapping IR codes to buttons
struct IRButton {
  uint32_t code;
  String name;
} irButtons[] = {
  {IR_CODE_UP, "Up"},
  {IR_CODE_DOWN, "Down"},
  {IR_CODE_OK, "OK"},
  {IR_CODE_BACK, "Back"},
  {IR_CODE_1, "1"},
  {IR_CODE_2, "2"},
  {IR_CODE_3, "3"},
  {IR_CODE_4, "4"},
  {IR_CODE_5, "5"},
  {IR_CODE_6, "6"},
  {IR_CODE_7, "7"},
  {IR_CODE_8, "8"},
  {IR_CODE_9, "9"}
};

// Total number of buttons
const int totalButtons = sizeof(irButtons) / sizeof(IRButton);

// Variables to handle IR button delay
unsigned long lastButtonPressTime = 0;
const unsigned long buttonPressInterval = 500; // 500 ms delay to improve responsiveness

// Fetch data interval
const unsigned long fetchInterval = 10; // Fetch data every 100ms

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing ESP32...");

  // Initialize EEPROM
  if (!EEPROM.begin(512)) {
    Serial.println("Failed to initialize EEPROM!");
    displayMessage("EEPROM Init Failed!");
    while (1);
  }

  // Initialize OLED display
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for most modules
    Serial.println("SSD1306 allocation failed");
    displayMessage("OLED Init Failed!");
    while (1); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.display();

  // Initialize IR receiver on pin 12
  irrecv.enableIRIn(); // Start the receiver
  Serial.println("IR Receiver is ready to capture codes.");

  // Initialize laser and galvo pins
  pinMode(RED_LASER_PIN, OUTPUT);
  pinMode(GREEN_LASER_PIN, OUTPUT);
  pinMode(BLUE_LASER_PIN, OUTPUT);
  pinMode(X_GALVO_PIN, OUTPUT);
  pinMode(Y_GALVO_PIN, OUTPUT);

  // Initialize DAC channels
  dacWrite(X_GALVO_PIN, 0);
  dacWrite(Y_GALVO_PIN, 0);

  // Load settings from EEPROM
  loadSettings();

  // For debugging: Print stored Wi-Fi credentials
  Serial.print("Stored SSID: ");
  Serial.println(storedSSID);
  Serial.print("Stored Password: ");
  Serial.println(storedPassword);
  Serial.print("Stored Python Server IP: ");
  Serial.println(storedPythonServerIP);

  // Initialize the effect task
  xTaskCreatePinnedToCore(
    EffectTask,          // Task function
    "EffectTask",        // Task name
    8192,                // Stack size (in bytes)
    NULL,                // Parameters
    1,                   // Priority
    &EffectTaskHandle,   // Task handle
    1                    // Core ID (1 for Core 1)
  );

  // Attempt to connect to Wi-Fi
  if (strlen(storedSSID) > 0 && strlen(storedPassword) > 0) {
    WiFi.begin(storedSSID, storedPassword);
    displayMessage("Connecting to Wi-Fi...");
    Serial.println("Attempting to connect to stored Wi-Fi...");
    Serial.print("SSID: ");
    Serial.println(storedSSID);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - startTime > 15000) { // 15-second timeout
        Serial.println("\nFailed to connect to stored Wi-Fi. Starting AP mode...");
        displayMessage("Failed to connect.\nStarting AP mode...");
        startAPMode();
        return;
      }
      delay(500);
      Serial.print(".");
    }

    // Connected to Wi-Fi
    Serial.println("\nConnected to Wi-Fi.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    displayMessage("Connected to " + String(storedSSID));

    // Start FetchDataTask AFTER Wi-Fi is connected
    xTaskCreate(
      FetchDataTask,       // Task function
      "FetchDataTask",     // Task name
      4096,                // Stack size
      NULL,                // Parameters
      1,                   // Priority
      &FetchDataTaskHandle // Task handle
    );

    startMainMode();
  } else {
    // No stored credentials, start AP mode
    startAPMode();
  }
}

void loop() {
  // Handle web server client requests
  server.handleClient();

  // If in main mode, handle IR remote and display status
  if (!isAPMode) {
    // Handle IR remote input
    handleIRRemote();

    // Display status on OLED
    displayStatus();
  }

  // Small delay to yield
  delay(1);
}

// FetchDataTask to handle data fetching
void FetchDataTask(void *pvParameters) {
  while (1) {
    if (WiFi.status() == WL_CONNECTED) {
      fetchData();
    }
    vTaskDelay(fetchInterval / portTICK_PERIOD_MS);
  }
}

// FreeRTOS Task to handle effects
void EffectTask(void *pvParameters) {
  while (1) {
    if (beatDropPending) {
      beatDropPending = false;
      beatDropActive = true;

      // Run beat drop effect
      int effectNumber = beatDropEffects[beatDropIndex];
      beatDropIndex = (beatDropIndex + 1) % 8; // Cycle through the 8 effects
      serialPrintln("Running beat drop effect " + String(effectNumber));

      // Run the effect with global playtime
      runEffect(effectNumber);

      beatDropActive = false;
    } else if (desiredEnergyLevel != currentEnergyLevel) {
      currentEnergyLevel = desiredEnergyLevel;

      // Run the effect for currentEnergyLevel
      if (currentEnergyLevel == 3) {
        // Energy level 3 cycles through 8 effects
        int effectNumber = energyLevel3Effects[energyLevel3Index];
        energyLevel3Index = (energyLevel3Index + 1) % 8; // Cycle through effects
        serialPrintln("Running energy level 3 effect " + String(effectNumber));

        runEffect(effectNumber);
      } else {
        int effectNumber = energyLevelEffects[currentEnergyLevel];
        serialPrintln("Running energy level effect " + String(effectNumber) + " for energy level " + String(currentEnergyLevel));

        if (effectNumber > 0) {
          // Run the effect
          runEffect(effectNumber);
        } else {
          // Effect number is 0, turn off lasers
          analogWrite(RED_LASER_PIN, 0);
          analogWrite(GREEN_LASER_PIN, 0);
          analogWrite(BLUE_LASER_PIN, 0);
        }
      }
    } else {
      // No new effect to run, sleep for a while
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}

// Helper function to translate encryption types
String translateEncryptionType(uint8_t encryptionType) {
  switch (encryptionType) {
    case WIFI_AUTH_OPEN:
      return "Open";
    case WIFI_AUTH_WEP:
      return "WEP";
    case WIFI_AUTH_WPA_PSK:
      return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA_WPA2_PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE:
      return "WPA2_ENTERPRISE";
    case WIFI_AUTH_WPA3_PSK:
      return "WPA3_PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK:
      return "WPA2_WPA3_PSK";
    case WIFI_AUTH_WAPI_PSK:
      return "WAPI_PSK";
    default:
      return "Unknown";
  }
}

// Start Access Point mode
void startAPMode() {
  Serial.println("Entering AP mode...");
  isAPMode = true;

  // Stop any existing Wi-Fi connections
  WiFi.disconnect(true, true); // Disconnect and erase credentials
  delay(1000); // Wait a moment

  // Start Access Point and enable station mode
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP32_AP", "123456789"); // Change SSID and password as needed
  IPAddress IP = WiFi.softAPIP();
  Serial.println("Access Point started.");
  Serial.print("AP IP Address: ");
  Serial.println(IP);

  // Update the top display with the AP IP address
  updateIPAddressDisplay();

  // Set up web server in AP mode
  server.on("/", HTTP_GET, handleRootAP);
  server.on("/connect", HTTP_POST, handleConnect);
  server.begin();
  Serial.println("AP Web Server started. Connect to http://" + IP.toString());

  // Display AP mode IP on OLED bottom part
  displayMessage("AP Mode IP:\n" + IP.toString());
}

// Handle root in AP mode
void handleRootAP() {
  // Scan for available networks
  int n = WiFi.scanNetworks();
  Serial.println("Scan done");

  // Build the HTML page
  String page = "<html><body><h1>Connect to Wi-Fi</h1>";
  page += "<form action='/connect' method='POST'>";

  if (n == 0) {
    Serial.println("No networks found");
    page += "<label>No networks found. Please refresh.</label><br>";
    page += "<label>SSID:</label>";
    page += "<input type='text' name='ssid' required><br>";
  } else {
    // Build dropdown menu
    page += "<label>SSID:</label>";
    page += "<select name='ssid'>";
    for (int i = 0; i < n; ++i) {
      // Get SSID and other details
      String ssid = WiFi.SSID(i);
      int32_t rssi = WiFi.RSSI(i);
      String encryptionType = translateEncryptionType(WiFi.encryptionType(i));
      // Escape special HTML characters in SSID to prevent issues
      ssid.replace("<", "&lt;");
      ssid.replace(">", "&gt;");
      ssid.replace("&", "&amp;");
      page += "<option value='" + ssid + "'>" + ssid + " (" + encryptionType + ", " + rssi + "dBm)</option>";
    }
    page += "</select><br>";
  }
  page += "<label>Password:</label><input type='password' name='password'><br>";
  page += "<input type='submit' value='Connect'>";
  page += "</form></body></html>";
  server.send(200, "text/html", page);
}

// Handle connect in AP mode
void handleConnect() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    // Validate inputs
    if (ssid.length() == 0) {
      server.send(200, "text/html", "<html><body><h1>SSID cannot be empty!</h1></body></html>");
      return;
    }

    // Attempt to connect to Wi-Fi
    WiFi.mode(WIFI_AP_STA); // Keep AP active during connection attempt
    WiFi.begin(ssid.c_str(), password.c_str());
    server.send(200, "text/html", "<html><body><h1>Connecting to " + ssid + "...</h1><p>Please wait...</p></body></html>");

    displayMessage("Connecting to " + ssid + "...");
    Serial.println("Attempting to connect to Wi-Fi...");

    // Wait for connection result
    int connResult = WiFi.waitForConnectResult(15000); // Wait up to 15 seconds
    if (connResult != WL_CONNECTED) {
      Serial.println("\nFailed to connect to Wi-Fi.");
      displayMessage("Connection failed!");

      // Provide more details
      String errorMessage = "<html><body><h1>Connection Failed!</h1>";
      errorMessage += "<p>Reason: ";
      switch (connResult) {
        case WL_NO_SSID_AVAIL:
          errorMessage += "SSID not available";
          break;
        case WL_CONNECT_FAILED:
          errorMessage += "Connection failed";
          break;
        case WL_CONNECTION_LOST:
          errorMessage += "Connection lost";
          break;
        case WL_DISCONNECTED:
          errorMessage += "Disconnected";
          break;
        default:
          errorMessage += "Unknown error";
          break;
      }
      errorMessage += "</p></body></html>";
      server.send(200, "text/html", errorMessage);
      return;
    }

    // Successful connection
    Serial.println("\nConnected to Wi-Fi!");
    IPAddress ip = WiFi.localIP();
    strncpy(storedSSID, ssid.c_str(), sizeof(storedSSID) - 1);
    storedSSID[sizeof(storedSSID) - 1] = '\0'; // Ensure null-terminated
    strncpy(storedPassword, password.c_str(), sizeof(storedPassword) - 1);
    storedPassword[sizeof(storedPassword) - 1] = '\0'; // Ensure null-terminated
    saveSettings();
    server.send(200, "text/html", "<html><body><h1>Connected! Rebooting...</h1></body></html>");
    displayMessage("Connected to " + ssid);

    // Reboot after saving credentials
    delay(2000);
    ESP.restart();
  } else {
    server.send(400, "text/html", "<html><body><h1>Bad Request</h1></body></html>");
  }
}

// Start Main mode
void startMainMode() {
  Serial.println("Entering Main mode...");
  isAPMode = false;

  // Update the top display with the local IP address
  updateIPAddressDisplay();

  // Set up web server in main mode
  server.on("/", HTTP_GET, handleRootMain);
  server.on("/set_effects", HTTP_POST, handleSetEffects);
  server.on("/set_energy_level3_effects", HTTP_POST, handleSetEnergyLevel3Effects);
  server.on("/set_python_server", HTTP_POST, handleSetPythonServerIP);
  server.on("/set_remote_effects", HTTP_POST, handleSetRemoteEffects);
  server.on("/set_beat_drop_effects", HTTP_POST, handleSetBeatDropEffects);
  server.on("/set_effect_playtime", HTTP_POST, handleSetEffectPlaytime);
  server.begin();
  Serial.println("Main web server started at IP: " + WiFi.localIP().toString());

  // Display status on OLED
  displayStatus();
}

// Handle root in Main mode
void handleRootMain() {
  // Build HTML for main web server
  String html = "<html><body><h1>Main Web Server</h1>";

  // Form to set energy level effects
  html += "<form action='/set_effects' method='post'>";
  html += "<h2>Set Effects for Energy Levels 0 to 2</h2>";
  for (int i = 0; i < 3; i++) {
    html += "Energy Level " + String(i) + ": <select name='energy" + String(i) + "'>";
    for (int j = 0; j <= 20; j++) { // Supports up to 20 effects
      html += "<option value='" + String(j) + "'";
      if (energyLevelEffects[i] == j) {
        html += " selected";
      }
      html += ">Effect " + String(j) + "</option>";
    }
    html += "</select><br>";
  }
  html += "<input type='submit' value='Save'></form><hr>";

  // Form to set Energy Level 3 Effects
  html += "<form action='/set_energy_level3_effects' method='post'>";
  html += "<h2>Set Effects for Energy Level 3</h2>";
  for (int i = 0; i < 8; i++) {
    html += "Energy Level 3 Effect " + String(i + 1) + ": <select name='energy3_" + String(i) + "'>";
    for (int j = 0; j <= 20; j++) {
      html += "<option value='" + String(j) + "'";
      if (energyLevel3Effects[i] == j) {
        html += " selected";
      }
      html += ">Effect " + String(j) + "</option>";
    }
    html += "</select><br>";
  }
  html += "<input type='submit' value='Save'></form><hr>";

  // Form to set global effect playtime
  html += "<form action='/set_effect_playtime' method='post'>";
  html += "<h2>Set Global Effect Playtime</h2>";
  html += "Current Playtime: <strong>" + String(effectPlaytime) + " seconds</strong><br>";
  html += "New Playtime (seconds): <input type='number' name='playtime' min='1' max='60' value='" + String(effectPlaytime) + "' required><br>";
  html += "<input type='submit' value='Save'></form><hr>";

  // Form to set Python Server IP
  html += "<form action='/set_python_server' method='post'>";
  html += "<h2>Set Python Server IP</h2>";
  html += "Current IP: <strong>" + String(storedPythonServerIP) + "</strong><br>";
  html += "New IP: <input type='text' name='python_ip' required><br>";
  html += "<input type='submit' value='Save'></form><hr>";

  // Form to set remote effects
  html += "<form action='/set_remote_effects' method='post'>";
  html += "<h2>Set Remote Effects</h2>";
  for (int i = 1; i <= 20; i++) {
    html += "Button " + String(i) + ": <select name='remote" + String(i) + "'>";
    for (int j = 0; j <= 20; j++) {
      html += "<option value='" + String(j) + "'";
      if (remoteEffects[i - 1] == j) {
        html += " selected";
      }
      html += ">Effect " + String(j) + "</option>";
    }
    html += "</select><br>";
  }
  html += "<input type='submit' value='Save'></form><hr>";

  // Form to set Beat Drop Effects
  html += "<form action='/set_beat_drop_effects' method='post'>";
  html += "<h2>Set Beat Drop Effects</h2>";
  for (int i = 0; i < 8; i++) {
    html += "Beat Drop Effect " + String(i + 1) + ": <select name='beatdrop" + String(i) + "'>";
    for (int j = 0; j <= 20; j++) { // Supports up to 20 effects
      html += "<option value='" + String(j) + "'";
      if (beatDropEffects[i] == j) {
        html += " selected";
      }
      html += ">Effect " + String(j) + "</option>";
    }
    html += "</select><br>";
  }
  html += "<input type='submit' value='Save'></form>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

// Handle setting energy level effects for levels 0 to 2
void handleSetEffects() {
  bool success = true;
  for (int i = 0; i < 3; i++) {
    String paramName = "energy" + String(i);
    if (server.hasArg(paramName)) {
      int value = server.arg(paramName).toInt();
      energyLevelEffects[i] = value;
    } else {
      success = false;
      break;
    }
  }
  if (success) {
    saveSettings();
    server.send(200, "text/html", "<html><body><h1>Energy Level Effects Saved</h1><a href='/'>Back</a></body></html>");
  } else {
    server.send(400, "text/html", "<html><body><h1>Bad Request</h1></body></html>");
  }
}

// Handle setting Energy Level 3 Effects
void handleSetEnergyLevel3Effects() {
  bool success = true;
  for (int i = 0; i < 8; i++) {
    String paramName = "energy3_" + String(i);
    if (server.hasArg(paramName)) {
      int value = server.arg(paramName).toInt();
      energyLevel3Effects[i] = value;
    } else {
      success = false;
      break;
    }
  }
  if (success) {
    saveSettings();
    server.send(200, "text/html", "<html><body><h1>Energy Level 3 Effects Saved</h1><a href='/'>Back</a></body></html>");
  } else {
    server.send(400, "text/html", "<html><body><h1>Bad Request</h1></body></html>");
  }
}

// Handle setting global effect playtime
void handleSetEffectPlaytime() {
  if (server.hasArg("playtime")) {
    int playtime = server.arg("playtime").toInt();
    if (playtime >= 1 && playtime <= 60) {
      effectPlaytime = playtime;
      saveSettings();
      server.send(200, "text/html", "<html><body><h1>Effect Playtime Saved</h1><a href='/'>Back</a></body></html>");
    } else {
      server.send(400, "text/html", "<html><body><h1>Playtime must be between 1 and 60 seconds.</h1></body></html>");
    }
  } else {
    server.send(400, "text/html", "<html><body><h1>Bad Request</h1></body></html>");
  }
}

// Handle setting Python server IP
void handleSetPythonServerIP() {
  if (server.hasArg("python_ip")) {
    String ip = server.arg("python_ip");
    strncpy(storedPythonServerIP, ip.c_str(), sizeof(storedPythonServerIP) - 1);
    storedPythonServerIP[sizeof(storedPythonServerIP) - 1] = '\0'; // Ensure null-terminated
    saveSettings();
    server.send(200, "text/html", "<html><body><h1>Python Server IP Saved</h1><a href='/'>Back</a></body></html>");
  } else {
    server.send(400, "text/html", "<html><body><h1>Bad Request</h1></body></html>");
  }
}

// Handle setting Beat Drop Effects
void handleSetBeatDropEffects() {
  bool success = true;
  for (int i = 0; i < 8; i++) {
    String paramName = "beatdrop" + String(i);
    if (server.hasArg(paramName)) {
      int value = server.arg(paramName).toInt();
      beatDropEffects[i] = value;
    } else {
      success = false;
      break;
    }
  }
  if (success) {
    saveSettings();
    server.send(200, "text/html", "<html><body><h1>Beat Drop Effects Saved</h1><a href='/'>Back</a></body></html>");
  } else {
    server.send(400, "text/html", "<html><body><h1>Bad Request</h1></body></html>");
  }
}

// Handle setting remote effects
void handleSetRemoteEffects() {
  bool success = true;
  for (int i = 1; i <= 20; i++) {
    String paramName = "remote" + String(i);
    if (server.hasArg(paramName)) {
      int value = server.arg(paramName).toInt();
      remoteEffects[i - 1] = value;
    } else {
      success = false;
      break;
    }
  }
  if (success) {
    saveSettings();
    server.send(200, "text/html", "<html><body><h1>Remote Effects Saved</h1><a href='/'>Back</a></body></html>");
  } else {
    server.send(400, "text/html", "<html><body><h1>Bad Request</h1></body></html>");
  }
}

// Save settings to EEPROM
void saveSettings() {
  Serial.println("Saving settings to EEPROM...");
  // Store SSID
  for (int i = 0; i < 32; i++) {
    EEPROM.write(EEPROM_SSID_ADDR + i, storedSSID[i]);
  }

  // Store Password
  for (int i = 0; i < 64; i++) {
    EEPROM.write(EEPROM_PASSWORD_ADDR + i, storedPassword[i]);
  }

  // Store Python Server IP
  for (int i = 0; i < 32; i++) {
    EEPROM.write(EEPROM_PYTHON_IP_ADDR + i, storedPythonServerIP[i]);
  }

  // Store Energy Level Effects (levels 0 to 2)
  for (int i = 0; i < 3; i++) {
    EEPROM.put(EEPROM_ENERGY_EFFECTS_ADDR + i * sizeof(int), energyLevelEffects[i]);
  }

  // Store Energy Level 3 Effects
  for (int i = 0; i < 8; i++) {
    EEPROM.put(EEPROM_ENERGY_LEVEL3_EFFECTS_ADDR + i * sizeof(int), energyLevel3Effects[i]);
  }

  // Store Remote Effects
  for (int i = 0; i < 20; i++) {
    EEPROM.put(EEPROM_REMOTE_EFFECTS_ADDR + i * sizeof(int), remoteEffects[i]);
  }

  // Store Beat Drop Effects
  for (int i = 0; i < 8; i++) {
    EEPROM.put(EEPROM_BEAT_DROP_EFFECTS_ADDR + i * sizeof(int), beatDropEffects[i]);
  }

  // Store Effect Playtime
  EEPROM.put(EEPROM_EFFECT_PLAYTIME_ADDR, effectPlaytime);

  EEPROM.commit();
  serialPrintln("Settings saved to EEPROM.");
}

// Load settings from EEPROM
void loadSettings() {
  Serial.println("Loading settings from EEPROM...");
  // Read SSID
  for (int i = 0; i < 32; i++) {
    storedSSID[i] = EEPROM.read(EEPROM_SSID_ADDR + i);
  }
  storedSSID[31] = '\0';  // Null-terminate the string

  // Read Password
  for (int i = 0; i < 64; i++) {
    storedPassword[i] = EEPROM.read(EEPROM_PASSWORD_ADDR + i);
  }
  storedPassword[63] = '\0';  // Null-terminate the string

  // Read Python Server IP
  for (int i = 0; i < 32; i++) {
    storedPythonServerIP[i] = EEPROM.read(EEPROM_PYTHON_IP_ADDR + i);
  }
  storedPythonServerIP[31] = '\0';  // Null-terminate the string

  // Read Energy Level Effects (levels 0 to 2)
  for (int i = 0; i < 3; i++) {
    EEPROM.get(EEPROM_ENERGY_EFFECTS_ADDR + i * sizeof(int), energyLevelEffects[i]);
  }

  // Read Energy Level 3 Effects
  for (int i = 0; i < 8; i++) {
    EEPROM.get(EEPROM_ENERGY_LEVEL3_EFFECTS_ADDR + i * sizeof(int), energyLevel3Effects[i]);
  }

  // Read Remote Effects
  for (int i = 0; i < 20; i++) {
    EEPROM.get(EEPROM_REMOTE_EFFECTS_ADDR + i * sizeof(int), remoteEffects[i]);
  }

  // Read Beat Drop Effects
  for (int i = 0; i < 8; i++) {
    EEPROM.get(EEPROM_BEAT_DROP_EFFECTS_ADDR + i * sizeof(int), beatDropEffects[i]);
  }

  // Read Effect Playtime
  EEPROM.get(EEPROM_EFFECT_PLAYTIME_ADDR, effectPlaytime);

  serialPrintln("Settings loaded from EEPROM.");
}

// Fetch data from Python server
void fetchData() {
  if (strlen(storedPythonServerIP) == 0) {
    if (!pythonIPNotSetPrinted) {
      serialPrintln("Python server IP not set.");
      pythonIPNotSetPrinted = true;  // Only print this once
    }
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    serialPrintln("Wi-Fi not connected.");
    return;
  }

  pythonIPNotSetPrinted = false;  // Reset once the IP is set
  String url = "http://" + String(storedPythonServerIP) + "/status";
  HTTPClient http;
  http.setTimeout(2000); // Set timeout to 2 seconds
  http.begin(url);
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<1024> doc; // Increase size if needed
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      // Parse JSON data
      float spotify_bpm = doc["spotify_bpm"];
      float volume_level = doc["volume_level"];
      int is_beat_drop = doc["is_beat_drop"];
      int energy_level = doc["energy_level"];
      int current_mode = doc["current_mode"];

      // Parse colors
      const char* color0 = doc["colors"][0];
      if (color0 != nullptr) {
        String hexColor = String(color0);
        uint8_t red, green, blue;
        parseHexColor(hexColor, red, green, blue);
        laserRed = red;
        laserGreen = green;
        laserBlue = blue;
      } else {
        // If colors[0] is not present, set default color
        laserRed = 255;
        laserGreen = 255;
        laserBlue = 255;
      }

      // Store current energy level and beat drop status
      currentBeatDrop = is_beat_drop;

      // Handle effects based on data
      handleEffects(is_beat_drop, energy_level);
    } else {
      serialPrintln("JSON deserialization failed: " + String(error.c_str()));
    }
  } else {
    serialPrintln("Error on HTTP request: " + String(httpResponseCode));
  }
  http.end();
}

// Function to parse hex color string to RGB
void parseHexColor(String hexColor, uint8_t &red, uint8_t &green, uint8_t &blue) {
  // Remove '#' if present
  if (hexColor.startsWith("#")) {
    hexColor = hexColor.substring(1);
  }
  if (hexColor.length() != 6) {
    // Invalid color format
    red = 255;
    green = 255;
    blue = 255;
    return;
  }
  red = strtol(hexColor.substring(0, 2).c_str(), NULL, 16);
  green = strtol(hexColor.substring(2, 4).c_str(), NULL, 16);
  blue = strtol(hexColor.substring(4, 6).c_str(), NULL, 16);
}

// Handle effects based on data
void handleEffects(int is_beat_drop, int energy_level) {
  if (is_beat_drop == 1) {
    beatDropPending = true;
  }
  desiredEnergyLevel = energy_level;
}

// Display status on OLED (lower part)
void displayStatus() {
  String message = "SSID: " + String(storedSSID) + "\n";
  message += "Energy Level: " + String(currentEnergyLevel) + "\n";
  message += "Beat Drop: " + String(currentBeatDrop);
  updateBottomDisplay(message);
}

// Update top part of the OLED display with IP address
void updateIPAddressDisplay() {
  String ipDisplay;
  if (isAPMode) {
    ipDisplay = "AP IP: " + WiFi.softAPIP().toString();
  } else {
    ipDisplay = "IP: " + WiFi.localIP().toString();
  }
  updateTopDisplay(ipDisplay);
}

// Update top part of the OLED display
void updateTopDisplay(String message) {
  // Clear only the top part of the display
  display.fillRect(0, TOP_START_Y, SCREEN_WIDTH, TOP_HEIGHT, SSD1306_BLACK);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, TOP_START_Y);
  display.println(message);
  display.display();
}

// Update bottom part of the OLED display
void updateBottomDisplay(String message) {
  // Clear only the bottom part of the display
  display.fillRect(0, BOTTOM_START_Y, SCREEN_WIDTH, BOTTOM_HEIGHT, SSD1306_BLACK);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, BOTTOM_START_Y);
  display.println(message);
  display.display();
}

// Display message on OLED (lower part)
void displayMessage(String message) {
  updateBottomDisplay(message);
}

// Serial print function that stores messages for OLED display
void serialPrintln(String message) {
  Serial.println(message);
  serialPrintBuffer[serialPrintIndex] = message;
  serialPrintIndex = (serialPrintIndex + 1) % SERIAL_PRINT_BUFFER_SIZE;
}

// Handle IR remote input with debounce
void handleIRRemote() {
  // Check if enough time has passed since the last button press
  if (millis() - lastButtonPressTime < buttonPressInterval) {
    return; // Ignore button presses within the debounce interval
  }

  // Check if an IR signal has been received
  if (irrecv.decode()) {
    uint32_t value = irrecv.decodedIRData.decodedRawData;

    Serial.print("IR Code received!\nHEX: ");
    Serial.print(value, HEX);
    Serial.print("\nDEC: ");
    Serial.println(value, DEC);

    // Update the last button press time
    lastButtonPressTime = millis();

    // Determine which button was pressed
    for (int i = 0; i < totalButtons; i++) {
      if (value == irButtons[i].code) {
        String buttonName = irButtons[i].name;
        Serial.println(buttonName);

        if (buttonName == "Up") {
          // Handle Up button if needed
        }
        else if (buttonName == "Down") {
          // Handle Down button if needed
        }
        else if (buttonName == "OK") {
          handleIRMenu();
        }
        else if (buttonName == "Back") {
          // Handle Back button if needed
          // Currently handled within the menu
        }
        else if (buttonName == "9") {
          // Button 9 pressed, reboot system without clearing EEPROM
          serialPrintln("Rebooting system...");
          displayMessage("Rebooting system...");
          delay(2000);
          ESP.restart();
        }
        else {
          // Handle number buttons for remote mode
          if (remoteMode) {
            int buttonNumber = buttonName.toInt() - 1; // 0-based index
            if (buttonNumber >= 0 && buttonNumber < 20) {
              runEffect(remoteEffects[buttonNumber]);

              // After effect execution, turn off lasers
              analogWrite(RED_LASER_PIN, 0);
              analogWrite(GREEN_LASER_PIN, 0);
              analogWrite(BLUE_LASER_PIN, 0);
            }
          }
        }

        break; // Exit the loop once the button is found
      }
    }

    irrecv.resume(); // Prepare to receive the next value
  }
}

// Handle IR remote menu with debounce and residual code clearing
void handleIRMenu() {
  Serial.println("Entering handleIRMenu()");

  // Disable IR receiver to clear any residual codes
  irrecv.disableIRIn();
  delay(100); // Wait for 100 milliseconds
  irrecv.enableIRIn();

  // Reset lastButtonPressTime to current time to ignore residual codes
  lastButtonPressTime = millis();

  // Display menu options
  int menuIndex = 0;
  const int numMenuOptions = 4;
  String menuOptions[] = {"Reconfigure Internet", "Reboot System", "Clear EEPROM", "Remote Mode"};
  bool menuActive = true;

  while (menuActive) {
    // Display current menu option
    String menuDisplay = "Menu:\n" + menuOptions[menuIndex];
    updateBottomDisplay(menuDisplay);

    // Check if enough time has passed since the last button press
    if (millis() - lastButtonPressTime >= buttonPressInterval) {
      if (irrecv.decode()) {
        uint32_t value = irrecv.decodedIRData.decodedRawData;

        Serial.print("Menu IR Code: ");
        Serial.println(value, HEX); // For debugging

        // Update the last button press time
        lastButtonPressTime = millis();

        // Determine which button was pressed
        bool buttonHandled = false;
        for (int i = 0; i < totalButtons; i++) {
          if (value == irButtons[i].code) {
            String buttonName = irButtons[i].name;

            if (buttonName == "Up") {
              menuIndex = (menuIndex - 1 + numMenuOptions) % numMenuOptions;
            }
            else if (buttonName == "Down") {
              menuIndex = (menuIndex + 1) % numMenuOptions;
            }
            else if (buttonName == "OK") {
              // Execute selected menu option
              switch (menuIndex) {
                case 0:
                  // Reconfigure Internet
                  Serial.println("Selected: Reconfigure Internet");

                  // Clear stored SSID and Password
                  memset(storedSSID, 0, sizeof(storedSSID));
                  memset(storedPassword, 0, sizeof(storedPassword));
                  saveSettings(); // Save the cleared credentials to EEPROM

                  Serial.println("Cleared stored Wi-Fi credentials.");

                  // Reboot the device to start in AP mode
                  serialPrintln("Rebooting to apply changes...");
                  displayMessage("Rebooting...");
                  delay(2000);
                  ESP.restart(); // Restart the ESP32

                  menuActive = false;
                  break;

                case 1:
                  // Reboot System
                  serialPrintln("Rebooting system...");
                  displayMessage("Rebooting system...");
                  delay(2000);
                  ESP.restart();
                  break;

                case 2:
                  // Clear EEPROM
                  Serial.println("Selected: Clear EEPROM");
                  clearEEPROM();
                  displayMessage("EEPROM cleared.");
                  menuActive = false;
                  break;

                case 3:
                  // Toggle Remote Mode
                  remoteMode = !remoteMode;
                  String msg = remoteMode ? "Remote mode ON" : "Remote mode OFF";
                  serialPrintln(msg);
                  displayMessage(msg);
                  menuActive = false;
                  break;
              }
            }
            else if (buttonName == "Back") {
              // Exit menu
              Serial.println("Selected: Back");
              menuActive = false;
            }

            buttonHandled = true;
            break; // Exit the loop once the button is handled
          }
        }

        if (!buttonHandled) {
          Serial.println("Unknown button pressed in menu.");
        }

        irrecv.resume(); // Prepare to receive the next value
      }
    }

    delay(100); // Small delay to prevent tight loop
  }

  // After exiting the menu, resume normal display
  displayStatus();

  Serial.println("Exiting handleIRMenu()");
}

// Function to clear EEPROM
void clearEEPROM() {
  Serial.println("Clearing EEPROM...");
  EEPROM.begin(512); // Ensure EEPROM is large enough
  for (int i = 0; i < 512; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  serialPrintln("EEPROM has been cleared!");
}

// Function to run the specified effect
void runEffect(int effectNumber) {
  unsigned long startTime = millis();
  switch (effectNumber) {
    case 1:
      effect1(effectPlaytime * 1000); // Convert seconds to milliseconds
      break;
    case 2:
      effect2(effectPlaytime * 1000);
      break;
    case 3:
      effect3(effectPlaytime * 1000);
      break;
    case 4:
      effect4(effectPlaytime * 1000);
      break;
    case 5:
      effect5(effectPlaytime * 1000);
      break;
    case 6:
      effect6(effectPlaytime * 1000);
      break;
    case 7:
      effect7(effectPlaytime * 1000);
      break;
    case 8:
      effect8(effectPlaytime * 1000);
      break;
    case 9:
      effect9(effectPlaytime * 1000);
      break;
    case 10:
      effect10(effectPlaytime * 1000);
      break;
    case 11:
      effect11(effectPlaytime * 1000);
      break;
    case 12:
      effect12(effectPlaytime * 1000);
      break;
    case 13:
      effect13(effectPlaytime * 1000);
      break;
    case 14:
      effect14(effectPlaytime * 1000);
      break;
    case 15:
      effect15(effectPlaytime * 1000);
      break;
    case 16:
      effect16(effectPlaytime * 1000);
      break;
    case 17:
      effect17(effectPlaytime * 1000);
      break;
    case 18:
      effect18(effectPlaytime * 1000);
      break;
    case 19:
      effect19(effectPlaytime * 1000);
      break;
    case 20:
      effect20(effectPlaytime * 1000);
      break;
    default:
      // Turn off lasers if effect number is invalid
      analogWrite(RED_LASER_PIN, 0);
      analogWrite(GREEN_LASER_PIN, 0);
      analogWrite(BLUE_LASER_PIN, 0);
      break;
  }
}

