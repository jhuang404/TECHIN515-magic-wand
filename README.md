# Magic Wand Gesture Recognition

This project implements a gesture recognition "magic wand" using an ESP32 microcontroller, MPU6050 accelerometer, and Edge Impulse machine learning. The system can recognize different gestures (Z, O, and V patterns) made with the wand and indicate the detected gesture through LED patterns.

## Hardware Requirements

- ESP32 development board
- MPU6050 accelerometer/gyroscope module
- Push button
- LED
- 220-330Ω resistor (for LED)
- Breadboard and jumper wires
- USB cable for programming
- Battery pack (3.7V Li-ion or 3xAAA batteries) for portable operation

## Wiring Diagram

Connect the components as follows:

| Component | ESP32 Pin |
|-----------|-----------|
| MPU6050 SDA | GPIO21 (default I2C) |
| MPU6050 SCL | GPIO22 (default I2C) |
| MPU6050 VCC | 3.3V |
| MPU6050 GND | GND |
| Button | GPIO20 |
| Button (other end) | GND |
| LED anode | GPIO5 (through resistor) |
| LED cathode | GND |

## Software Dependencies

This project requires the following libraries:

1. Edge Impulse Library for your trained model
   - Generated specifically for your project (jhuang404-project-510_LAB4_inferencing.h)

2. Adafruit MPU6050 Library
   - Can be installed from Arduino Library Manager

3. Adafruit Sensor Library
   - Can be installed from Arduino Library Manager

4. Wire Library (built into Arduino IDE)

## Installation Instructions

1. **Install the Arduino IDE**
   - Download and install the latest version from [arduino.cc](https://www.arduino.cc/en/software)

2. **Install ESP32 Board Support**
   - Open Arduino IDE
   - Go to File > Preferences
   - Add the following URL to "Additional Boards Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to Tools > Board > Boards Manager
   - Search for "ESP32" and install the ESP32 board support package

3. **Install Required Libraries**
   - Go to Tools > Manage Libraries
   - Search for and install:
     - "Adafruit MPU6050"
     - "Adafruit Unified Sensor"

4. **Download and Import Edge Impulse Library**
   - Download your Edge Impulse library for Arduino from your Edge Impulse project
   - In Arduino IDE, go to Sketch > Include Library > Add .ZIP Library
   - Select the downloaded ZIP file

## Uploading the Code

1. Connect your ESP32 to your computer via USB
2. Select the correct board from Tools > Board menu
3. Select the correct COM port from Tools > Port menu
4. Click the Upload button (right arrow icon)

## Using the Magic Wand

1. **USB Powered Mode**
   - Connect ESP32 to computer or USB power source
   - Open Serial Monitor (115200 baud) to view detailed information
   - The LED will flash to indicate system startup and successful initialization

2. **Battery Powered Mode**
   - Connect battery pack to the ESP32
   - The LED will flash to indicate system startup
   - No serial output will be available

3. **Operation**
   - Press the button to start capturing gesture data
   - Move the wand in a Z, O, or V pattern while the button is pressed
   - After 1 second, the system will automatically process the gesture
   - The LED will display a pattern corresponding to the recognized gesture:
     - Z gesture: 3 quick flashes
     - O gesture: 2 slow flashes
     - V gesture: 1 long flash (1 second)

## Troubleshooting

1. **System doesn't start or LED doesn't light**
   - Check power connections
   - Verify LED polarity and resistor value
   - Ensure ESP32 is properly powered (3.3V)

2. **MPU6050 not detected**
   - Check I2C connections (SDA and SCL)
   - Verify power connections to MPU6050
   - Try decreasing I2C clock speed in code

3. **Button not working**
   - Verify button connections
   - Check that button pin is properly defined in code
   - Ensure button is properly connected to GND

4. **No gesture recognition or poor accuracy**
   - Verify that features array size matches your model requirements
   - Ensure MPU6050 is properly calibrated
   - Try retraining model with more examples

5. **Battery operation issues**
   - Add capacitors (100-1000μF) between power supply and ground
   - Verify battery voltage is sufficient (>3.3V)
   - Reduce CPU frequency if necessary

## Notes on Edge Impulse Integration

The code may display the following message when running inference:
```
INFO: Impulse maintains state. Call run_classifier_init() to reset state (e.g. if data stream is interrupted.)
```

This is normal behavior from the Edge Impulse library and not an error. The code is designed to handle this by:
- Clearing the features array before each new capture
- Using local variables for signal and result structures
- Maintaining a clean data flow for each inference operation

## Customizing the Code

- To change pin assignments, modify the `BUTTON_PIN` and `LED_PIN` definitions
- To adjust sensitivity, modify the `SAMPLE_RATE_MS` and `CAPTURE_DURATION_MS` values
- To add new gestures, retrain your Edge Impulse model and update the code to handle new labels

## License

This project is released under the MIT License.
