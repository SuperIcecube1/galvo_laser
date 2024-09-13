// Include necessary libraries
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <IRremote.hpp> // Ensure IRremote 4.x is installed
#include <HTTPClient.h>
#include <ArduinoJson.h>

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
#define SDA_PIN 22
#define SCL_PIN 23
#define RED_LASER_PIN 18
#define BLUE_LASER_PIN 19
#define GREEN_LASER_PIN 21
#define X_GALVO_PIN 26
#define Y_GALVO_PIN 25
#define IR_RECEIVER_PIN 12 // Pin for the IR receiver

// EEPROM addresses
#define EEPROM_SSID_ADDR 0
#define EEPROM_PASSWORD_ADDR 64
#define EEPROM_PYTHON_IP_ADDR 128
#define EEPROM_ENERGY_EFFECTS_ADDR 192
#define EEPROM_REMOTE_EFFECTS_ADDR 256

// Wi-Fi network credentials
char storedSSID[32] = "";
char storedPassword[64] = "";
char storedPythonServerIP[32] = "";

// Energy level effects and remote effects
int energyLevelEffects[4] = {0, 0, 0, 0}; // Initialize to 0
int remoteEffects[20] = {0};              // Initialize to 0

// Web server
WebServer server(80);

// State variables
bool isAPMode = false;
bool remoteMode = false;

// IR receiver
IRrecv irrecv(IR_RECEIVER_PIN);

// Function prototypes
void startAPMode();
void startMainMode();
void saveSettings();
void loadSettings();
void fetchData();
void handleEffects(int is_beat_drop, int energy_level);
void displayStatus(); // Updated function
void runEffect(int effectNumber);
void handleIRMenu();
void clearEEPROM();
void serialPrintln(String message);
void updateIPAddressDisplay();
void handleIRRemote();
void displayMessage(String message); // Added prototype

// Other variables
unsigned long previousFetchTime = 0;
const long fetchInterval = 500; // Fetch data every 500ms

// Serial print buffer for OLED display
#define SERIAL_PRINT_BUFFER_SIZE 10
String serialPrintBuffer[SERIAL_PRINT_BUFFER_SIZE];
int serialPrintIndex = 0;
int displayLine = 0;

// Flag to prevent repeated "Python server IP not set" messages
bool pythonIPNotSetPrinted = false;

// Variables to store current energy level and beat drop status
int currentEnergyLevel = -1;
int currentBeatDrop = 0;

// Define IR Codes based on your remote
const uint32_t IR_CODE_UP    = 0xB946FF00;
const uint32_t IR_CODE_DOWN  = 0xEA15FF00;
const uint32_t IR_CODE_OK    = 0xBF40FF00;
const uint32_t IR_CODE_BACK  = 0xBB44FF00;
const uint32_t IR_CODE_1     = 0xE916FF00;
const uint32_t IR_CODE_2     = 0xF20DFF00;
const uint32_t IR_CODE_3     = 0xE21DFF00;
const uint32_t IR_CODE_4     = 0xA857FF00;
const uint32_t IR_CODE_5     = 0xE619FF00;
const uint32_t IR_CODE_6     = 0xF807FF00;
const uint32_t IR_CODE_7     = 0xF609FF00;
const uint32_t IR_CODE_8     = 0xA05F0000;
const uint32_t IR_CODE_9     = 0x9867FF00;

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
  pinMode(BLUE_LASER_PIN, OUTPUT);
  pinMode(GREEN_LASER_PIN, OUTPUT);
  pinMode(X_GALVO_PIN, OUTPUT);
  pinMode(Y_GALVO_PIN, OUTPUT);

  // Load settings from EEPROM
  loadSettings();

  // For debugging: Print stored Wi-Fi credentials
  Serial.print("Stored SSID: ");
  Serial.println(storedSSID);
  Serial.print("Stored Password: ");
  Serial.println(storedPassword);
  Serial.print("Stored Python Server IP: ");
  Serial.println(storedPythonServerIP);

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
    startMainMode();
  } else {
    // No stored credentials, start AP mode
    startAPMode();
  }
}

void loop() {
  // Handle web server client requests
  server.handleClient();

  // If in main mode, fetch data and handle IR remote
  if (!isAPMode) {
    // Fetch data from Python server periodically
    unsigned long currentMillis = millis();
    if (currentMillis - previousFetchTime >= fetchInterval) {
      previousFetchTime = currentMillis;
      fetchData();
    }

    // Handle IR remote input
    handleIRRemote();

    // Display status on OLED
    displayStatus();
  }
}

// Start Access Point mode
void startAPMode() {
  Serial.println("Entering AP mode...");
  isAPMode = true;

  // Stop any existing Wi-Fi connections
  WiFi.disconnect(true, true); // Disconnect and erase credentials
  delay(1000); // Wait a moment

  // Start Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32_AP", "123456789");
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

// Start Main mode
void startMainMode() {
  Serial.println("Entering Main mode...");
  isAPMode = false;

  // Update the top display with the local IP address
  updateIPAddressDisplay();

  // Set up web server in main mode
  server.on("/", HTTP_GET, handleRootMain);
  server.on("/set_effects", HTTP_POST, handleSetEffects);
  server.on("/set_python_server", HTTP_POST, handleSetPythonServerIP);
  server.on("/set_remote_effects", HTTP_POST, handleSetRemoteEffects);
  server.begin();
  Serial.println("Main web server started at IP: " + WiFi.localIP().toString());

  // Display status on OLED
  displayStatus();
}

// Handle root in AP mode
void handleRootAP() {
  // Serve a web page to collect Wi-Fi credentials
  String page = "<html><body><h1>Connect to Wi-Fi</h1>";
  page += "<form action='/connect' method='POST'>";
  page += "<label>SSID:</label>";
  page += "<input type='text' name='ssid' required><br>";
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
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    server.send(200, "text/html", "<html><body><h1>Connecting to " + ssid + "...</h1></body></html>");

    displayMessage("Connecting to " + ssid + "...");
    Serial.println("Attempting to connect to Wi-Fi...");

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - startTime > 15000) { // 15-second timeout
        server.send(200, "text/html", "<html><body><h1>Connection Failed!</h1></body></html>");
        Serial.println("Connection failed!");
        displayMessage("Connection failed!");
        return;
      }
      delay(500);
      Serial.print(".");
    }

    // Successful connection
    Serial.println("\nConnected to Wi-Fi!");
    IPAddress ip = WiFi.localIP();
    strncpy(storedSSID, ssid.c_str(), sizeof(storedSSID) - 1);
    strncpy(storedPassword, password.c_str(), sizeof(storedPassword) - 1);
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

// Handle root in Main mode
void handleRootMain() {
  // Build HTML for main web server
  String html = "<html><body><h1>Main Web Server</h1>";
  html += "<form action='/set_effects' method='post'>";
  html += "<h2>Set Effects for Energy Levels</h2>";
  for (int i = 0; i < 4; i++) {
    html += "Energy Level " + String(i) + ": <select name='energy" + String(i) + "'>";
    for (int j = 0; j <= 20; j++) { // Assuming 20 effects, plus option 0 for blank
      html += "<option value='" + String(j) + "'";
      if (energyLevelEffects[i] == j) {
        html += " selected";
      }
      html += ">Effect " + String(j) + "</option>";
    }
    html += "</select><br>";
  }
  html += "<input type='submit' value='Save'></form>";

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
  html += "<input type='submit' value='Save'></form>";

  html += "<form action='/set_python_server' method='post'>";
  html += "<h2>Set Python Server IP</h2>";
  html += "IP Address: <input type='text' name='python_ip' value='" + String(storedPythonServerIP) + "' required><br>";
  html += "<input type='submit' value='Save'></form>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

// Handle setting energy level effects
void handleSetEffects() {
  bool success = true;
  for (int i = 0; i < 4; i++) {
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

// Handle setting Python server IP
void handleSetPythonServerIP() {
  if (server.hasArg("python_ip")) {
    String ip = server.arg("python_ip");
    strncpy(storedPythonServerIP, ip.c_str(), sizeof(storedPythonServerIP) - 1);
    saveSettings();
    server.send(200, "text/html", "<html><body><h1>Python Server IP Saved</h1><a href='/'>Back</a></body></html>");
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

  // Store Energy Level Effects
  for (int i = 0; i < 4; i++) {
    EEPROM.put(EEPROM_ENERGY_EFFECTS_ADDR + i * sizeof(int), energyLevelEffects[i]);
  }

  // Store Remote Effects
  for (int i = 0; i < 20; i++) {
    EEPROM.put(EEPROM_REMOTE_EFFECTS_ADDR + i * sizeof(int), remoteEffects[i]);
  }

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

  // Read Energy Level Effects
  for (int i = 0; i < 4; i++) {
    EEPROM.get(EEPROM_ENERGY_EFFECTS_ADDR + i * sizeof(int), energyLevelEffects[i]);
  }

  // Read Remote Effects
  for (int i = 0; i < 20; i++) {
    EEPROM.get(EEPROM_REMOTE_EFFECTS_ADDR + i * sizeof(int), remoteEffects[i]);
  }

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

  pythonIPNotSetPrinted = false;  // Reset once the IP is set
  String url = "http://" + String(storedPythonServerIP) + "/status";
  HTTPClient http;
  http.begin(url);
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      float spotify_bpm = doc["spotify_bpm"];
      float volume_level = doc["volume_level"];
      int is_beat_drop = doc["is_beat_drop"];
      int energy_level = doc["energy_level"];
      int current_mode = doc["current_mode"];

      // Store current energy level and beat drop status
      currentEnergyLevel = energy_level;
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

// Handle effects based on data
void handleEffects(int is_beat_drop, int energy_level) {
  if (is_beat_drop == 1) {
    // Run beat drop effect
    runEffect(energyLevelEffects[3]);  // Assuming beat drop uses the highest energy level effect
  } else if (energy_level >= 0 && energy_level <= 3) {
    int effectNumber = energyLevelEffects[energy_level];
    if (effectNumber > 0) {
      runEffect(effectNumber);
    } else {
      // Effect number is 0, turn off lasers
      digitalWrite(RED_LASER_PIN, LOW);
      digitalWrite(BLUE_LASER_PIN, LOW);
      digitalWrite(GREEN_LASER_PIN, LOW);
    }
  } else if (energy_level == -1) {
    // Turn off lasers
    digitalWrite(RED_LASER_PIN, LOW);
    digitalWrite(BLUE_LASER_PIN, LOW);
    digitalWrite(GREEN_LASER_PIN, LOW);
  }
}

// Run the specified effect
void runEffect(int effectNumber) {
  serialPrintln("Running effect " + String(effectNumber));
  // Placeholder for running the effect
  // Implement your effects here
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

// Serial print function that stores messages for OLED display
void serialPrintln(String message) {
  Serial.println(message);
  serialPrintBuffer[serialPrintIndex] = message;
  serialPrintIndex = (serialPrintIndex + 1) % SERIAL_PRINT_BUFFER_SIZE;
  // Update the bottom display to show the latest message
  // Commented out to prevent interference with status display
  // updateBottomDisplay(message);
}

// Display message on OLED (lower part)
void displayMessage(String message) {
  updateBottomDisplay(message);
}
