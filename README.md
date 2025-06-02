# ESP32-S3 Bluetooth RC Car

## Project Overview
This project implements a remote-controlled car using an ESP32-S3 microcontroller and the Bluepad32 library for Bluetooth game controller connectivity. It features an LCD for status display, dual motor control with an HR8833 driver, selectable speed modes, and a NeoPixel LED for visual status indication.

## Features
- **Bluetooth Controller Input:** Uses a standard Bluetooth game controller (via Bluepad32 library) for intuitive remote control.
- **Dual Motor Control:** Controls two DC motors using an HR8833 motor driver (or compatible DIR/PWM driver), enabling tank-style steering.
- **LCD1602 Display:** Shows real-time information:
    - Bluetooth connection status (Connected, Disconnected, Controller Name).
    - Current speed percentage for each motor (MA and MB).
    - Notification of speed mode changes.
- **Selectable Speed Modes:** Cycle through LOW, MEDIUM, and HIGH speed settings using a controller button to adjust maneuverability.
- **NeoPixel LED Status Indicator:** Provides visual feedback on system status:
    - Green: System booting/initializing.
    - Blue: Controller connected.
    - Red: Controller disconnected or waiting for connection.
- **Configurable Pin Layout:** Clearly defined pins for easy hardware setup and modification.

## Hardware Requirements
- ESP32-S3 development board (e.g., ESP32-S3-DevKitC-1)
- Bluetooth Game Controller (compatible with Bluepad32, e.g., PS3, PS4, Nintendo Switch Pro Controller, generic Bluetooth gamepads)
- LCD1602 Display with I2C module (ensure the I2C address is 0x27 as defined by `LCD_ADDR` in the code, or modify if different)
- HR8833 Dual Motor Driver (or a similar driver like L298N, TB6612FNG, if it can be controlled with separate Speed (PWM) and Direction pins per motor)
- Two DC Motors (suitable for your RC car chassis and the HR8833 driver)
- Adafruit NeoPixel LED (single WS2812B LED or a small strip, connected to the pin defined by `NEOPIXEL_PIN` in the code)
- Breadboard and jumper wires
- Appropriate power supply for the ESP32-S3 and motors. (Motors should ideally have a separate power supply or a robust supply shared with the ESP32 if current requirements are managed).
- RC car chassis (optional, for a complete build)

## Wiring Instructions
Ensure all connections are secure. It's recommended to power the motors from a separate power supply that can handle their current draw, especially if using larger motors. Make sure to connect the ground of the motor power supply to the ESP32-S3 ground.

### Pin Connections
| Component         | ESP32-S3 Pin | Macro Name in Code | Description                      |
|-------------------|--------------|--------------------|----------------------------------|
| LCD I2C SDA       | GPIO 8       | `SDA_PIN`          | I2C Data Line                    |
| LCD I2C SCL       | GPIO 9       | `SCL_PIN`          | I2C Clock Line                   |
| Motor A - Speed   | GPIO 10      | `MA_SPEED_PIN`     | PWM signal for Motor A speed     |
| Motor A - Direction | GPIO 11      | `MA_DIR_PIN`       | Direction control for Motor A    |
| Motor B - Speed   | GPIO 12      | `MB_SPEED_PIN`     | PWM signal for Motor B speed     |
| Motor B - Direction | GPIO 13      | `MB_DIR_PIN`       | Direction control for Motor B    |
| NeoPixel LED      | GPIO 48      | `NEOPIXEL_PIN`     | Data line for WS2812B LED        |

**Note on Motor Driver (HR8833):** The code assumes a motor driver configuration where each motor (A and B) is controlled by one pin for Speed (PWM) and one pin for Direction.
- For HR8833, this typically means:
    - Motor A Speed (ESP32 GPIO 10) -> HR8833 ENA or PWMA (or equivalent speed control pin for channel A)
    - Motor A Direction (ESP32 GPIO 11) -> HR8833 IN1 (with IN2 tied LOW/HIGH or controlled by another pin if more complex logic is needed. For simple DIR pin: HIGH=Forward, LOW=Backward, this pin acts as the primary direction input).
    - Motor B Speed (ESP32 GPIO 12) -> HR8833 ENB or PWMB
    - Motor B Direction (ESP32 GPIO 13) -> HR8833 IN3 (with IN4 handled similarly to IN2).
- You may need to adjust wiring or code slightly if your specific HR8833 breakout or usage differs. The key is one PWM for speed and one digital pin for direction per motor.
- Ensure common ground (GND) between ESP32, motor driver, and motor power supply.

## Software Setup
This project is designed for the Arduino IDE environment, targeting an ESP32-S3 board. You'll need to install several libraries to compile the code.

Ensure you have the ESP32 board support package installed in your Arduino IDE. If not, you can typically add it via `File > Preferences > Additional Boards Manager URLs` (search for ESP32 Arduino core instructions for the specific URL).

### Required Arduino Libraries
- **Bluepad32:** For Bluetooth game controller support.
    - Author: Ricardo Quesada
    - Installation: Search for "Bluepad32" in the Arduino IDE Library Manager and install. Ensure you have the ESP32 board definitions installed.
- **LiquidCrystal_I2C:** For controlling the I2C LCD1602 display.
    - Author: Frank de Brabander (or a compatible version by another author, e.g., Marco Schwartz)
    - Installation: Search for "LiquidCrystal_I2C" in the Library Manager and install.
- **Adafruit NeoPixel:** For controlling the WS2812B NeoPixel LED.
    - Author: Adafruit
    - Installation: Search for "Adafruit NeoPixel" in the Library Manager and install.
- **LibPrintf:** Used for enhanced `printf` formatting capabilities to the Serial console.
    - Author: Peter Brier (Note: This might sometimes be bundled or automatically available with certain ESP32 core versions or Bluepad32 examples. If you get compilation errors related to `printf` or `LibPrintf.h` not found, install this explicitly.)
    - Installation: Search for "LibPrintf" in the Library Manager and install.
- **Wire:** Standard Arduino library for I2C communication.
    - This library is usually included with the Arduino IDE and ESP32 board support package, so no separate installation is typically needed.

## Compilation and Upload
1.  **Connect ESP32-S3:** Connect your ESP32-S3 board to your computer via USB.
2.  **Select Board:** In the Arduino IDE, go to `Tools > Board` and select your specific ESP32-S3 model (e.g., "ESP32S3 Dev Module" or similar).
3.  **Select Port:** Go to `Tools > Port` and choose the COM port corresponding to your ESP32-S3.
4.  **Configure Board Settings (if necessary):** For some ESP32-S3 boards, you might need to enable PSRAM if your board has it and the code utilizes it (this project does not explicitly require PSRAM). Ensure "USB CDC On Boot" is enabled if you want Serial output after boot without manual reset on some boards.
5.  **Verify/Compile:** Click the "Verify" button (checkmark icon) to compile the sketch and check for errors.
6.  **Upload:** Click the "Upload" button (right arrow icon) to upload the compiled code to the ESP32-S3. You might need to hold down the "BOOT" button on your ESP32-S3 board while clicking "Upload" if it doesn't enter programming mode automatically, then release "BOOT" once the upload process begins.

## Usage Instructions

### Controller Pairing
1.  **Power On:** Power on the ESP32-S3 RC car.
2.  **Controller Pairing Mode:** Put your Bluetooth game controller into pairing mode. The method varies by controller (e.g., for a PS4 controller, hold the PS button and Share button until the light bar flashes rapidly).
3.  **Automatic Pairing:** The ESP32 (Bluepad32) will automatically scan for and attempt to connect to nearby controllers in pairing mode if it doesn't have a previously bonded device.
4.  **Connection Indication:**
    - The LCD will display "Ctrl Connected" and the controller's name.
    - The NeoPixel LED will turn BLUE.
5.  **Reconnecting Bonded Devices:** Once a controller is successfully paired (bonded), Bluepad32 will attempt to reconnect to it automatically on subsequent power-ups if the controller is on and in range.
6.  **Forcing New Pairing:** If you want to pair a new controller or are having trouble, you can uncomment the `BP32.forgetBluetoothKeys();` line in the `setup()` function of the `esp32_bluepad32_lcd.ino` sketch, upload the code, run it once, then comment the line out again and re-upload. This clears stored Bluetooth keys.

### Controls
- **Left Joystick (Vertical Y-axis):** Controls forward and backward speed of the car.
    - Push Up (negative values, towards -512): Move forward. Speed is proportional to joystick deflection, scaled by the current Speed Mode.
    - Pull Down (positive values, towards 512): Move backward. Speed is proportional to joystick deflection, scaled by the current Speed Mode.
    - Center (or within dead zone): Motors stop.
- **D-Pad Left (Button mask `0x0010`):** Makes the car pivot turn right (Left motor forward, Right motor backward).
    - Pressing this button overrides forward/backward joystick input for turning.
    - Turning speed is fixed but also scaled by the current Speed Mode.
- **D-Pad Right (Button mask `0x0020`):** Makes the car pivot turn left (Left motor backward, Right motor forward).
    - Pressing this button overrides forward/backward joystick input for turning.
    - Turning speed is fixed but also scaled by the current Speed Mode.
- **'Y' Button (North Button on Gamepad - e.g., Triangle on PS controller, typically `BUTTON_Y`):** Cycles through speed modes:
    - **LOW:** Reduced maximum speed (50%) for both throttle and turning.
    - **MEDIUM:** Default maximum speed (75%) for both throttle and turning.
    - **HIGH:** Full maximum speed (100%) for both throttle and turning.
    - The LCD will briefly display the selected mode.
- **Note:** The Left Joystick X-axis is no longer used for primary steering with this control scheme. It will still trigger the temporary LCD diagnostic display if moved.

### LCD Display
The LCD provides real-time status:
- **Line 1 (Normal Operation):**
    - Shows controller connection status (e.g., "Ctrl: [Controller Name]", "Not Connected").
    - Briefly displays the current speed mode when changed (e.g., "Speed: LOW"), then reverts to controller status.
- **Line 2 (Normal Operation):**
    - Displays the current speed percentage being applied to each motor: "MA: XX% MB: YY%".
- **Temporary Input Display (Diagnostic):**
    - When new button or joystick input is detected (after any dead-zone processing), the display will briefly (for 2 seconds) show:
        - **Line 1:** Raw button data (e.g., "Btns:0x1234").
        - **Line 2:** Processed joystick values (e.g., "LX:100 LY:-50").
    - After 2 seconds, the display reverts to the normal status lines described above. This is useful for checking controller inputs and dead-zone effects.

### NeoPixel LED Status
The onboard NeoPixel LED indicates the system status:
- **Green:** System is booting up or initializing.
- **Blue:** Bluetooth controller is successfully connected.
- **Red:** Bluetooth controller is disconnected or the system is waiting for a connection.

## Basic Troubleshooting
- **Controller Not Connecting:**
    - Ensure your controller is charged and in pairing mode (see controller's manual).
    - Check if the ESP32's NeoPixel LED is Red (indicating it's waiting for connection).
    - Try moving the controller closer to the ESP32, avoiding interference.
    - If you've paired a controller before and want to pair a new one, or if pairing fails repeatedly, try uncommenting `BP32.forgetBluetoothKeys();` in `setup()`, upload, run once, then comment it out and re-upload. This clears stored Bluetooth device information from the ESP32.
    - Check the Serial Monitor output in the Arduino IDE for any error messages from Bluepad32.

- **Motors Not Working or Moving Incorrectly:**
    - **No Movement:**
        - Verify all motor wiring to the HR8833 driver and from the driver to the motors.
        - Ensure the motor driver (HR8833) has power. Motors often require a separate, higher current power supply than what the ESP32 USB connection can provide. Make sure the motor PSU ground is connected to the ESP32 ground.
        - Check if the controller is connected (Blue NeoPixel LED). Motors will not operate if the controller is disconnected.
    - **Incorrect Direction:**
        - If motors run in the opposite direction to what's expected, you can either swap the two wires for that specific motor or invert the logic for the `forward` parameter in the `controlMotorA()` or `controlMotorB()` functions in the code. For example, change `digitalWrite(MA_DIR_PIN, forward ? HIGH : LOW);` to `digitalWrite(MA_DIR_PIN, forward ? LOW : HIGH);` for the respective motor.

- **LCD Not Displaying or Showing Garbled Text:**
    - **No Display:**
        - Double-check the I2C wiring: SDA (GPIO 8, defined as `SDA_PIN`) and SCL (GPIO 9, defined as `SCL_PIN`) to the LCD.
        - Ensure the LCD has power and ground connected.
        - Verify the I2C address of your LCD module. The code uses `0x27` (defined as `LCD_ADDR`). If your module has a different address, you'll need to change `LCD_ADDR` in the `.ino` file. You can use an I2C scanner sketch to find the address.
    - **Garbled Text:**
        - This can sometimes be due to poor connections or incorrect I2C address.
        - Ensure the `lcd.init()` and `lcd.backlight()` calls are present in `setup()`.

- **Compilation Errors:**
    - **Library Not Found:** Ensure all libraries listed in the "Software Setup" section are correctly installed in your Arduino IDE.
    - **Board Not Selected:** Make sure you have selected the correct ESP32-S3 board model under `Tools > Board`.
    - Check the error messages in the Arduino IDE console for specific details.

- **NeoPixel LED Not Working:**
    - Verify the `NEOPIXEL_PIN` (GPIO 48) is correctly wired to the NeoPixel's data input (DIN).
    - Ensure the NeoPixel has 5V power and ground. If using a strip, the power requirement might be higher.
    - The code controls only the first LED (`NEOPIXEL_NUM = 1`).

- **Serial Monitor Issues:**
    - Ensure the correct COM port is selected and the baud rate in the Serial Monitor window is set to 115200 (or as configured in `Serial.begin()`).
    - If you enabled Telnet or Bluetooth console, USB Serial output might be minimal or disabled for `printf`.
