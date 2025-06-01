// Copyright 2021 - 2023 Ricardo Quesada
// SPDX-License-Identifier: Apache-2.0

// Configurable options.
// #define ENABLE_MULTICORE // Experimental, might be unstable.
// #define ENABLE_BT_CONSOLE // When defined, it will use Bluetooth Serial Console instead of USB one.
// #define ENABLE_TELNET_CONSOLE // When defined, it will use Telnet Console instead of USB one.
// #define ENABLE_OTA_SUPPORT // When defined, it will enable support for OTA updates.

// Pins for LCD I2C. These are the "official" ESP32 I2C pins.
#define SDA_PIN 21
#define SCL_PIN 22

#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// Pins for motor control
#define MA_SPEED_PIN 10
#define MA_DIR_PIN 11
#define MB_SPEED_PIN 12
#define MB_DIR_PIN 13

// Pins for NeoPixel. Only one LED is supported.
#define NEOPIXEL_PIN 48      // Digital pin connected to the NeoPixel
#define NEOPIXEL_NUM 1       // Number of LEDs in the strip (usually 1 for on-board)

// --- END OF CONFIGURABLE OPTIONS ---

#if defined(ENABLE_BT_CONSOLE) && defined(ENABLE_TELNET_CONSOLE)
#error "Cannot enable BT Console and Telnet Console at the same time"
#endif

#if defined(ENABLE_BT_CONSOLE) || defined(ENABLE_TELNET_CONSOLE)
#define ENABLE_CONSOLE
#endif

#include <Arduino.h>
#include <Bluepad32.h>
#include <LibPrintf.h> // Only needed for debug

#include <Wire.h>                  // For I2C communication (LCD)
#include <LiquidCrystal_I2C.h>     // For I2C LCD control

#include <Adafruit_NeoPixel.h>     // For NeoPixel LED control

// NeoPixel object
Adafruit_NeoPixel strip(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// For OTA updates
#ifdef ENABLE_OTA_SUPPORT
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Update.h>
// WiFi credentials. Set your SSID and password.
// Leave them blank if you don't want to use OTA updates.
const char* ssid = "";
const char* password = "";
#endif // ENABLE_OTA_SUPPORT

// For Bluetooth Console
#ifdef ENABLE_BT_CONSOLE
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;
#endif // ENABLE_BT_CONSOLE

// LCD object
// LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS); // Instantiated later

// For Telnet Console
#ifdef ENABLE_TELNET_CONSOLE
#include <WiFi.h>
// Telnet server.
WiFiServer telnetServer(23);
WiFiClient serverClient;
#endif // ENABLE_TELNET_CONSOLE

// ESP32 entry point.
// Arduino setup function. Will be called once.
void setup();
// Arduino loop function. Will be called repeatedly.
void loop();

// Sanity checks
#ifndef CONFIG_BLUEPAD32_MAX_DEVICES
#error "CONFIG_BLUEPAD32_MAX_DEVICES not defined. You might need to select a different board from the menu"
#endif // CONFIG_BLUEPAD32_MAX_DEVICES

//
// README FIRST, README FIRST, README FIRST
//
// Bluepad32 has a built-in interactive console.
// By default it is mapped to the USB-serial port.
//
// It is used to dump useful debugging information. It is recommended to keep it enabled.
//
// But it can be disabled by defining the macro UNI_DISABLE_CONSOLE.
// Read the console documentation here:
// https://ricardoquesada.github.io/unijoysticle2/esp32/uni_console.html
//
// // Uncomment to disable console
// #define UNI_DISABLE_CONSOLE

// Definitions for the Console
#ifdef ENABLE_CONSOLE
Console* console = nullptr;

// Defines the UART Console.
class SerialConsole : public Console {
public:
    SerialConsole() {}
    size_t write(uint8_t c) override { return Serial.write(c); }
    size_t write(const uint8_t* buffer, size_t size) override { return Serial.write(buffer, size); }
};

#ifdef ENABLE_BT_CONSOLE
// Defines the Bluetooth Console
class BTConsole : public Console {
public:
    BTConsole() {}
    size_t write(uint8_t c) override { return SerialBT.write(c); }
    size_t write(const uint8_t* buffer, size_t size) override { return SerialBT.write(buffer, size); }
};
#endif // ENABLE_BT_CONSOLE

#ifdef ENABLE_TELNET_CONSOLE
// Defines the Telnet Console
class TelnetConsole : public Console {
public:
    TelnetConsole() {}
    size_t write(uint8_t c) override {
        if (serverClient && serverClient.connected()) {
            return serverClient.write(c);
        }
        return 0;
    }
    size_t write(const uint8_t* buffer, size_t size) override {
        if (serverClient && serverClient.connected()) {
            return serverClient.write(buffer, size);
        }
        return 0;
    }
};
#endif // ENABLE_TELNET_CONSOLE
#endif // ENABLE_CONSOLE

// LCD object (16 columns, 2 rows)
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// --- Global Variables for Controller and RC Car State ---
ControllerPtr myControllers[CONFIG_BLUEPAD32_MAX_DEVICES]; // Array to hold connected controller objects
static Bluepad32BP32 BP32;                                 // Bluepad32 primary object

// Speed Mode Definitions
enum SpeedMode { MODE_LOW, MODE_MEDIUM, MODE_HIGH }; // Enum for different speed levels
SpeedMode currentSpeedMode = MODE_MEDIUM;            // Current selected speed mode, default to medium
const float speedFactors[] = {0.5f, 0.75f, 1.0f};    // Speed scaling factors for LOW, MEDIUM, HIGH modes
unsigned int prev_buttons_for_mode_switch = 0;       // Previous button state for mode switch debouncing

// Button mask for speed mode switching.
// BP32_BUTTON_Y is typically the 'Y' button on Nintendo-style controllers or Triangle on PlayStation.
// If BP32_BUTTON_Y is not defined by your Bluepad32 version or board, use its hex value e.g. 0x0008.
const unsigned int MODE_SWITCH_BUTTON_MASK = BP32_BUTTON_Y;

// --- Controller Connection Callbacks ---

// onConnectedController: Called when a new controller connects.
void onConnectedController(ControllerPtr ctl) {
    bool found = false;
    for (int i = 0; i < CONFIG_BLUEPAD32_MAX_DEVICES; i++) {
        if (myControllers[i] == nullptr) { // Find an empty slot for the new controller
            ConsoleContextHolder ctx(*console); // Ensure console output is synchronized
            printf("CALLBACK: Controller connected, index=%d\n", i);
            // You can get more controller properties here if needed:
            // GamepadProperties properties = ctl->getProperties();
            // printf("Controller model: %s, VID=0x%04x, PID=0x04%x\n", ctl->getModelName(), properties.vendor_id, properties.product_id);

            myControllers[i] = ctl; // Store the controller object

            if (i == 0) { // Special handling for the first controller (myControllers[0])
                // Initialize button state for mode switching to prevent false trigger on first press
                prev_buttons_for_mode_switch = ctl->buttons();
            }

            // Update LCD
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Ctrl Connected");
            String controllerName = ctl->getModelName().c_str();
            lcd.setCursor(0, 1);
            lcd.print(controllerName.substring(0, LCD_COLS)); // Print name, truncated to fit LCD

            // Update NeoPixel status LED to Blue
            strip.setPixelColor(0, strip.Color(0, 0, 255)); // Blue
            strip.show();

            found = true;
            break;
        }
    }
    if (!found) {
        ConsoleContextHolder ctx(*console);
        printf("CALLBACK: Controller connected, but no empty slot available.\n");
    }
}

// onDisconnectedController: Called when a controller disconnects.
void onDisconnectedController(ControllerPtr ctl) {
    bool found = false;
    for (int i = 0; i < CONFIG_BLUEPAD32_MAX_DEVICES; i++) {
        if (myControllers[i] == ctl) { // Find the disconnected controller
            ConsoleContextHolder ctx(*console);
            printf("CALLBACK: Controller disconnected from index=%d\n", i);
            myControllers[i] = nullptr; // Remove from active controllers

            if (i == 0) { // Special handling if the first controller disconnected
                // Reset button state for mode switching
                prev_buttons_for_mode_switch = 0;
            }

            // Update LCD
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Ctrl Disconnected");
            lcd.setCursor(0, 1);
            lcd.print("Waiting for pair");

            // Stop motors for safety
            stopMotors();

            // Update NeoPixel status LED to Red
            strip.setPixelColor(0, strip.Color(255, 0, 0)); // Red
            strip.show();

            found = true;
            break;
        }
    }
    if (!found) {
        ConsoleContextHolder ctx(*console);
        printf("CALLBACK: Controller disconnected, but not found in myControllers array.\n");
    }
}

// processGamepad: Called periodically for each connected controller.
// Currently used for detailed serial diagnostics of button and axis states.
void processGamepad(ControllerPtr ctl) {
    // Example of iterating through all buttons and printing their state
    for (int i = 0; i < BUTTON_MAX; i++) { // BUTTON_MAX is defined in Bluepad32 library
        if (ctl->isButtonPressed(i))
            printf("Button %s pressed (%#04x)\n", ctl->getButtonName(i), ctl->buttons()); // Also log raw button mask
        if (ctl->isButtonReleased(i))
            printf("Button %s released (%#04x)\n", ctl->getButtonName(i), ctl->buttons());
    }

    // Axis reports values from -512 to 511.
    printf("Axis LX: %4d, LY: %4d, RX: %4d, RY: %4d, Brake: %4d, Throttle: %4d, Misc: %4d\n",
           ctl->axisX(), ctl->axisY(), ctl->axisRX(), ctl->axisRY(), ctl->brake(), ctl->throttle(), ctl->miscButton());

    // Example of using controller's Player LEDs and Rumble.
    // This section can be customized or removed if not needed.
    static TickType_t prev_rumble_tick = 0;
    TickType_t current_tick = xTaskGetTickCount();
    if (current_tick - prev_rumble_tick > 3000 / portTICK_PERIOD_MS) { // Every 3 seconds
        prev_rumble_tick = current_tick;

        // Some gamepads might not support rumble.
        // ctl->setRumble(100, 100); // Duration in ms, L+R intensity (0-255)

        // Set Player LEDs (if the gamepad has them)
        // Example: ctl->setPlayerLEDs(0b0110); // Sets 2nd and 3rd LEDs

        // Check for Misc button press (e.g., PS, Xbox, Home button)
        if (ctl->isMiscButtonPressed()) {
            printf("Misc button pressed\n");
        }
    }
}


// --- Arduino Setup Function ---
void setup() {
    // --- Serial Console Initialization ---
    // Enables serial output for debugging. Can be USB, Bluetooth, or Telnet.
#if defined(ENABLE_CONSOLE)
#if defined(ENABLE_BT_CONSOLE)
    SerialBT.begin("ESP32-bluepad32"); // Bluetooth device name
    console = new BTConsole();
    printf_init(console);
    printf("Bluetooth Serial Console enabled\n");
#elif defined(ENABLE_TELNET_CONSOLE)
    printf("Connecting to %s ", ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        printf(".");
    }
    printf("\nWiFi connected\n");
    printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    telnetServer.begin();
    telnetServer.setNoDelay(true);
    console = new TelnetConsole();
    printf_init(console);
    printf("Telnet Console enabled. Connect to IP: %s\n", WiFi.localIP().toString().c_str());
#else // Default to UART console
    Serial.begin(115200); // Initialize USB Serial
    console = new SerialConsole();
    printf_init(console); // Initialize printf() to use the console
    printf("UART Serial Console enabled\n");
#endif
#else // Console disabled
    Serial.begin(115200); // Still init Serial for basic Arduino Serial.println
    Serial.println("Console is disabled. Uncomment UNI_DISABLE_CONSOLE to enable advanced console features.");
#endif

    // --- Hardware Initialization ---
    // LCD
    lcd.init();        // Initialize the LCD
    lcd.backlight();   // Turn on the backlight
    lcd.clear();       // Clear any previous content
    lcd.setCursor(0, 0);
    lcd.print("RC Car Booting...");
    lcd.setCursor(0, 1);
    lcd.print("Scanning BT...");

    // NeoPixel LED
    strip.begin();                  // Initialize NeoPixel strip
    strip.setPixelColor(0, strip.Color(0, 255, 0)); // Set to Green during boot
    strip.show();                   // Update the LED
    strip.setBrightness(50);        // Set brightness (0-255)

    // Motor Control Pins
    pinMode(MA_SPEED_PIN, OUTPUT);  // Motor A Speed (PWM)
    pinMode(MA_DIR_PIN, OUTPUT);    // Motor A Direction
    pinMode(MB_SPEED_PIN, OUTPUT);  // Motor B Speed (PWM)
    pinMode(MB_DIR_PIN, OUTPUT);    // Motor B Direction

    // Ensure motors are stopped at boot
    stopMotors();
    printf("Motors stopped at boot.\n");

    // --- Bluepad32 Initialization ---
    // Setup Bluepad32 callbacks for controller connection events
    BP32.setup(&onConnectedController, &onDisconnectedController);

    // Optional: Factory reset Bluetooth keys. Useful for debugging pairing issues.
    // BP32.forgetBluetoothKeys();

    // Start Bluepad32 (on Core 0 or 1, depending on config)
#ifdef UNI_BLUEPAD32_DUAL_CORE
    printf("Bluepad32 running on Core 1 (Dual Core mode).\n");
#else
    printf("Bluepad32 running on Core 0 (Single Core mode).\n");
#endif
    BP32.begin(); // Start Bluepad32 task

    // --- OTA (Over-The-Air Updates) Initialization ---
#ifdef ENABLE_OTA_SUPPORT
    ArduinoOTA.setHostname("esp32-bluepad32-rc-car"); // Set a unique hostname
    ArduinoOTA.begin();
    printf("OTA updates enabled. Hostname: %s\n", ArduinoOTA.getHostname().c_str());
#endif
    printf("Setup complete.\n");
}


// --- Arduino Main Loop ---
void loop() {
    // --- System Tasks ---
#ifdef ENABLE_OTA_SUPPORT
    ArduinoOTA.handle(); // Handle Over-The-Air update requests
#endif
#ifdef ENABLE_TELNET_CONSOLE
    if (telnetServer.hasClient()) { // Handle new Telnet client connections
        if (serverClient && serverClient.connected()) serverClient.stop();
        serverClient = telnetServer.available();
        if (serverClient && serverClient.connected()) {
            serverClient.write("\033[2J"); // Clear telnet screen
            printf("Telnet client connected.\n");
        }
    }
#endif

    // --- Bluepad32 Task ---
    BP32.update(); // Process controller input and Bluetooth events. Must be called frequently.

    // --- Process Input from All Connected Controllers (for diagnostics) ---
    // This loop calls processGamepad for each connected controller, which prints detailed info to Serial.
    for (int i = 0; i < CONFIG_BLUEPAD32_MAX_DEVICES; i++) {
        if (myControllers[i] && myControllers[i]->isConnected()) {
            // processGamepad(myControllers[i]); // Uncomment for verbose diagnostics for ALL controllers
        }
    }
     if (myControllers[0] && myControllers[0]->isConnected()) { // Only run diagnostics for controller 0 to reduce spam
        // processGamepad(myControllers[0]);
     }


    // --- RC Car Control Logic (operates on myControllers[0]) ---

    // Joystick and Motor Control Constants
    const int JOYSTICK_DEAD_ZONE = 25;  // Ignore small joystick movements (~5% of 511)
    const int JOYSTICK_MAX_VALUE = 511; // Max value from Bluepad32 joystick axis

    // Shared variables for motor PWM values, accessible by LCD update logic
    static int pwmLeft = 0;  // Use static to retain value if controller disconnects mid-loop for LCD
    static int pwmRight = 0; // Use static to retain value if controller disconnects mid-loop for LCD

    if (myControllers[0] != nullptr && myControllers[0]->isConnected()) {
        // --- Speed Mode Switching (uses Y/Triangle button) ---
        unsigned int current_buttons = myControllers[0]->buttons();
        if ((current_buttons & MODE_SWITCH_BUTTON_MASK) && !(prev_buttons_for_mode_switch & MODE_SWITCH_BUTTON_MASK)) {
            currentSpeedMode = (SpeedMode)((currentSpeedMode + 1) % 3); // Cycle: LOW -> MEDIUM -> HIGH -> LOW
            printf("Speed mode changed to: %d\n", currentSpeedMode);

            // Briefly display new mode on LCD Line 1 (will be overwritten by timed LCD update)
            char lcd_buffer_mode[LCD_COLS + 1];
            const char* modeNames[] = {"Speed: LOW", "Speed: MEDIUM", "Speed: HIGH"};
            lcd.setCursor(0, 0);
            int len_mode = snprintf(lcd_buffer_mode, LCD_COLS + 1, "%s", modeNames[currentSpeedMode]);
            lcd.print(lcd_buffer_mode);
            for (int k = len_mode; k < LCD_COLS; k++) lcd.print(" ");
        }
        prev_buttons_for_mode_switch = current_buttons; // Update previous button state for next iteration

        // --- Joystick Input Processing ---
        int stickY = myControllers[0]->axisY(); // Left stick Y-axis for forward/backward
        int stickX = myControllers[0]->axisX(); // Left stick X-axis for turning

        // Apply Dead Zone to ignore minor joystick drift
        if (abs(stickY) < JOYSTICK_DEAD_ZONE) stickY = 0;
        if (abs(stickX) < JOYSTICK_DEAD_ZONE) stickX = 0;

        // --- Motor Speed Calculation (Tank Control) ---
        float normalizedY = (float)stickY / JOYSTICK_MAX_VALUE; // Normalize to -1.0 to 1.0
        float normalizedX = (float)stickX / JOYSTICK_MAX_VALUE; // Normalize to -1.0 to 1.0

        // Tank steering: Y for speed, X for direction differential
        float motorLeftSpeedNormalized = normalizedY - normalizedX;
        float motorRightSpeedNormalized = normalizedY + normalizedX;

        // Clamp normalized speeds to the range [-1.0, 1.0]
        motorLeftSpeedNormalized = constrain(motorLeftSpeedNormalized, -1.0, 1.0);
        motorRightSpeedNormalized = constrain(motorRightSpeedNormalized, -1.0, 1.0);

        // Map normalized speed to PWM range (0-255)
        pwmLeft = (int)(abs(motorLeftSpeedNormalized) * 255);
        pwmRight = (int)(abs(motorRightSpeedNormalized) * 255);

        // Apply current speed mode scaling factor
        pwmLeft = (int)((float)pwmLeft * speedFactors[currentSpeedMode]);
        pwmRight = (int)((float)pwmRight * speedFactors[currentSpeedMode]);

        // Final PWM value clamp (should not be necessary with current factors but good practice)
        pwmLeft = constrain(pwmLeft, 0, 255);
        pwmRight = constrain(pwmRight, 0, 255);

        // Determine motor direction
        bool forwardLeft = motorLeftSpeedNormalized >= 0;
        bool forwardRight = motorRightSpeedNormalized >= 0;

        // --- Actuate Motors ---
        controlMotorA(pwmLeft, forwardLeft);
        controlMotorB(pwmRight, forwardRight);

    } else { // If controller myControllers[0] is not connected
        // Ensure PWM values are zero for LCD display and motors are stopped
        // (stopMotors() in onDisconnectedController also handles this)
        pwmLeft = 0;
        pwmRight = 0;
        // stopMotors(); // Redundant if onDisconnectedController is reliable, but ensures safety.
                       // Let's rely on onDisconnectedController and setup() for explicit stop commands.
    }

    // --- Timed LCD Update ---
    // Updates the LCD periodically to show status and motor speeds.
    static unsigned long lastLcdUpdateTime = 0;
    const int lcdUpdateInterval = 200; // Update LCD 5 times per second (200ms)

    if (millis() - lastLcdUpdateTime > lcdUpdateInterval) {
        lastLcdUpdateTime = millis(); // Reset the timer
        char lcd_buffer[LCD_COLS + 1]; // Buffer for formatting LCD lines
        int len;

        // Display Controller Status/Name on Line 1
        // This will overwrite the brief speed mode message if it was shown.
        lcd.setCursor(0, 0);
        if (myControllers[0] != nullptr && myControllers[0]->isConnected()) {
            String name = myControllers[0]->getModelName().c_str();
            // Format: "Ctrl: [Name]", ensure name is truncated to fit.
            len = snprintf(lcd_buffer, LCD_COLS + 1, "Ctrl: %.*s", LCD_COLS - 6, name.c_str());
        } else {
            len = snprintf(lcd_buffer, LCD_COLS + 1, "Not Connected");
        }
        lcd.print(lcd_buffer);
        for (int i = len; i < LCD_COLS; i++) lcd.print(" "); // Clear rest of the line

        // Display Motor Speeds on Line 2
        lcd.setCursor(0, 1);
        int speedPercentA = (int)((float)pwmLeft / 255.0 * 100.0);
        int speedPercentB = (int)((float)pwmRight / 255.0 * 100.0);
        // Format: "MA:XXX% MB:YYY%". %3d pads with spaces for numbers < 3 digits.
        snprintf(lcd_buffer, LCD_COLS + 1, "MA:%3d%% MB:%3d%%", speedPercentA, speedPercentB);
        lcd.print(lcd_buffer); // This format is exactly 16 chars, no need to clear.
    }

    // Add a small delay to yield to other tasks (e.g., WiFi, Bluetooth stack)
    delay(20);
}


// --- Motor Control Helper Functions ---

// controlMotorA: Sets speed and direction for Motor A.
// - speed: PWM value (0-255).
// - forward: true for forward, false for backward.
void controlMotorA(int speed, bool forward) {
    // Assumption: HIGH on DIR_PIN means forward, LOW means backward. Adjust if needed.
    digitalWrite(MA_DIR_PIN, forward ? HIGH : LOW);
    analogWrite(MA_SPEED_PIN, speed); // analogWrite handles PWM on ESP32
}

// controlMotorB: Sets speed and direction for Motor B.
// - speed: PWM value (0-255).
// - forward: true for forward, false for backward.
void controlMotorB(int speed, bool forward) {
    digitalWrite(MB_DIR_PIN, forward ? HIGH : LOW);
    analogWrite(MB_SPEED_PIN, speed);
}

// stopMotors: Stops both motors by setting their speed to 0.
void stopMotors() {
    controlMotorA(0, true); // Direction is irrelevant when speed is 0
    controlMotorB(0, true);
    // printf("Motors STOPPED.\n"); // Optional: Log to serial
    // Consider updating LCD here if a persistent "MOTORS STOPPED" message is desired,
    // but current design has timed updates in loop() that would overwrite it.
}
