#include <jhuang404-project-510_LAB4_inferencing.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// ========== Pin Definitions ==========
#define BUTTON_PIN 20
#define LED_PIN 5          

// ========== Sensor & Sampling ==========
Adafruit_MPU6050 mpu;

#define SAMPLE_RATE_MS 10
#define CAPTURE_DURATION_MS 1000
#define FEATURE_SIZE EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

bool capturing = false;
unsigned long last_sample_time = 0;
unsigned long capture_start_time = 0;
int sample_count = 0;
float features[FEATURE_SIZE];
bool last_button_state = HIGH;

// Flags to track boot process
volatile bool boot_complete = false;
unsigned long power_on_time = 0;

// ========== Feature Callback ==========
int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

// ========== Boot indication function ==========
void indicate_boot_stage(int num_blinks, int blink_speed) {
    for (int i = 0; i < num_blinks; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(blink_speed);
        digitalWrite(LED_PIN, LOW);
        delay(blink_speed);
    }
    delay(500); // Pause between stages
}

// ========== Setup ==========
void setup() {
    // Very first step - configure output pin and blink once to show power
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // LED ON to show we have power
    delay(1000);                  // Keep it on for a full second
    digitalWrite(LED_PIN, LOW);   // Turn off
    
    power_on_time = millis();
    
    // Configure button pin (with internal pull-up)
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Initialize serial but don't wait for it
    Serial.begin(115200);
    
    // Display boot stage 1
    indicate_boot_stage(1, 200); // 1 blink for boot stage 1
    
    // Initialize I2C with explicit pins (helps on some ESP32 variants)
    Wire.begin();
    
    // Display boot stage 2
    indicate_boot_stage(2, 200); // 2 blinks for boot stage 2
    
    // Initialize MPU6050 with simplified error handling
    bool sensor_ok = false;
    for (int attempt = 0; attempt < 3; attempt++) {
        if (mpu.begin()) {
            sensor_ok = true;
            break;
        }
        delay(500);
    }
    
    if (sensor_ok) {
        // MPU6050 successfully initialized
        indicate_boot_stage(3, 200); // 3 blinks for successful sensor init
        
        // Configure MPU6050 with minimal settings
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    } else {
        // MPU6050 failed to initialize
        indicate_boot_stage(5, 100); // 5 fast blinks for sensor failure
    }
    
    // System is now ready
    digitalWrite(LED_PIN, HIGH);  // Ready indication
    delay(2000);                  // Longer LED ON to indicate ready state
    digitalWrite(LED_PIN, LOW);
    
    boot_complete = true;
    
    // Debug info to serial (if available)
    if (Serial) {
        Serial.println("System boot complete");
    }
}

// ========== Main Loop ==========
void loop() {
    // Safety delay after boot
    if (millis() - power_on_time < 3000) {
        return;  // Wait 3 seconds after power on before accepting input
    }
    
    // Read the button with debounce
    static unsigned long last_debounce_time = 0;
    static bool last_reading = HIGH;
    
    bool reading = digitalRead(BUTTON_PIN);
    
    // If the reading has changed, reset debounce timer
    if (reading != last_reading) {
        last_debounce_time = millis();
    }
    last_reading = reading;
    
    // Only accept button press if state is stable for 50ms
    if ((millis() - last_debounce_time) > 50) {
        // Button state changed
        if (reading != last_button_state) {
            last_button_state = reading;
            
            // Button pressed (LOW)
            if (reading == LOW && !capturing) {
                digitalWrite(LED_PIN, HIGH);
                delay(50);
                digitalWrite(LED_PIN, LOW);
                
                capturing = true;
                sample_count = 0;
                capture_start_time = millis();
                last_sample_time = millis();
                
                if (Serial) {
                    Serial.println("Button pressed, capturing");
                }
            }
        }
    }
    
    // If we're capturing data
    if (capturing) {
        if (millis() - last_sample_time >= SAMPLE_RATE_MS) {
            last_sample_time = millis();
            
            // Get data from MPU6050
            sensors_event_t a, g, temp;
            mpu.getEvent(&a, &g, &temp);
            
            // Store data if we have space
            if (sample_count < FEATURE_SIZE / 3) {
                int idx = sample_count * 3;
                features[idx] = a.acceleration.x;
                features[idx + 1] = a.acceleration.y;
                features[idx + 2] = a.acceleration.z;
                sample_count++;
            }
            
            // Check if capture duration has elapsed
            if (millis() - capture_start_time >= CAPTURE_DURATION_MS) {
                capturing = false;
                
                // Show we completed capture with a quick double blink
                digitalWrite(LED_PIN, HIGH);
                delay(100);
                digitalWrite(LED_PIN, LOW);
                delay(100);
                digitalWrite(LED_PIN, HIGH);
                delay(100);
                digitalWrite(LED_PIN, LOW);
                
                if (Serial) {
                    Serial.println("Capture complete, running inference");
                }
                
                // Run the inference
                run_inference();
            }
        }
    }
    
    // Small delay to reduce CPU usage
    delay(10);
}

// ========== Inference ==========
void run_inference() {
    ei_impulse_result_t result = { 0 };
    signal_t features_signal;
    features_signal.total_length = FEATURE_SIZE;
    features_signal.get_data = &raw_feature_get_data;

    EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);
    if (res != EI_IMPULSE_OK) {
        if (Serial) {
            Serial.println("Classifier error");
        }
        
        // Error pattern: rapid blinks
        for (int i = 0; i < 5; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(50);
            digitalWrite(LED_PIN, LOW);
            delay(50);
        }
        return;
    }

    print_inference_result(result);
}

// ========== Show Results ==========
void print_inference_result(ei_impulse_result_t result) {
    float max_value = 0;
    int max_index = -1;

    // Find highest confidence prediction
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > max_value) {
            max_value = result.classification[i].value;
            max_index = i;
        }
    }

    if (max_index != -1) {
        const char* label = ei_classifier_inferencing_categories[max_index];
        
        if (Serial) {
            Serial.print("Prediction: ");
            Serial.print(label);
            Serial.print(" (");
            Serial.print(max_value * 100);
            Serial.println("%)");
        }

        // Show result with LED pattern
        if (strcmp(label, "Z") == 0) {
            for (int i = 0; i < 3; i++) {
                digitalWrite(LED_PIN, HIGH);
                delay(100);
                digitalWrite(LED_PIN, LOW);
                delay(100);
            }
        } else if (strcmp(label, "O") == 0) {
            for (int i = 0; i < 2; i++) {
                digitalWrite(LED_PIN, HIGH);
                delay(400);
                digitalWrite(LED_PIN, LOW);
                delay(400);
            }
        } else if (strcmp(label, "V") == 0) {
            digitalWrite(LED_PIN, HIGH);
            delay(1000);
            digitalWrite(LED_PIN, LOW);
        }
    }
}
