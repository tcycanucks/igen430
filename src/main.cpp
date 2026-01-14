#include <Arduino.h>
#include "driver/twai.h"
#include <Adafruit_PM25AQI.h> 

// --- CAN BUS CONFIGURATION ---
#define CAN_TX_PIN 5
#define CAN_RX_PIN 4
#define TRANSMIT_INTERVAL_MS 1000

// Define two distinct CAN IDs for the data streams
#define CAN_ID_PM_CONCENTRATION 0x100 // ID for PM 1.0, 2.5, 10 environmental concentrations
#define CAN_ID_PM_AQI_US        0x101 // ID for PM 2.5 AQI and PM 10 AQI values

// --- PM SENSOR CONFIGURATION ---
// Pins: PM Sensor Pin: TX - TX2/GPIO 17, RX - RX2/GPIO 16 (for Serial1)
#define PM_RX_PIN 16 // Connects to Sensor TX
#define PM_TX_PIN 17 // Connects to Sensor RX

// --- GLOBAL VARIABLES ---
Adafruit_PM25AQI aqi = Adafruit_PM25AQI();
unsigned long previousMillis = 0; // store last time message was sent

// --- FUNCTION PROTOTYPES ---
void setup_can();
void setup_pm_sensor();
void read_and_transmit_pm_data();

// =========================================================================
// SETUP FUNCTION
// =========================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize the CAN bus driver
  setup_can(); 
  
  // Initialize the PM sensor via UART
  setup_pm_sensor();
}

// =========================================================================
// LOOP FUNCTION
// =========================================================================
void loop() {
  unsigned long currentMillis = millis();

  // Check if it's time to send a message (e.g., every 1000ms)
  if (currentMillis - previousMillis >= TRANSMIT_INTERVAL_MS) {
    previousMillis = currentMillis;

    // Read the PM sensor and transmit the data over CAN
    read_and_transmit_pm_data();
  }
}

// =========================================================================
// CONFIGURATION FUNCTIONS
// =========================================================================

void setup_can() {
    Serial.println("Initializing TWAI (CAN)…");
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
      Serial.println("CAN Driver installed");
    } else {
      Serial.println("Failed to install CAN driver");
      // Consider an infinite loop or reset here if CAN is critical
      return; 
    }

    // Start driver
    if (twai_start() == ESP_OK) {
      Serial.println("CAN Driver started");
    } else {
      Serial.println("Failed to start CAN driver");
      // Consider an infinite loop or reset here if CAN is critical
      return;
    }
}

// -------------------------------------------------------------------------

void setup_pm_sensor() {
  Serial.println("Adafruit PMSA003I Air Quality Sensor");
  
  // Serial1.begin(BaudRate, Configuration, RX_Pin, TX_Pin)
  // RX_Pin (16) is where the sensor TX is connected
  // TX_Pin (17) is where the sensor RX is connected
  Serial1.begin(9600, SERIAL_8N1, PM_RX_PIN, PM_TX_PIN); 

  // Wait three seconds for sensor to boot up!
  delay(3000);

  // Connectivity: Only attempt UART connection
  if (! aqi.begin_UART(&Serial1)) { 
    Serial.println("Could not find PM 2.5 sensor via UART! Halting.");
    while (1) delay(10);
  }
  Serial.println("PM25 sensor found via UART!");
}

// =========================================================================
// TRANSMISSION FUNCTION
// =========================================================================

// Helper function to transmit a single CAN message and report status
esp_err_t transmit_can_message(uint32_t identifier, uint8_t data_len, uint8_t data[]) {
  twai_message_t message;
  message.identifier = identifier;
  message.extd = 0; // Standard ID
  message.data_length_code = data_len;
  memcpy(message.data, data, data_len);

  // Wait up to 100ms for a free transmission slot
  esp_err_t status = twai_transmit(&message, pdMS_TO_TICKS(100)); 

  if (status == ESP_OK) {
      Serial.print("Message 0x");
      Serial.print(identifier, HEX);
      Serial.println(" queued for transmission");
  } else {
      Serial.print("Failed to queue message 0x");
      Serial.print(identifier, HEX);
      Serial.print(": ");
      Serial.println(esp_err_to_name(status));
  }
  return status;
}

// Main function to read sensor and prepare/send two messages
void read_and_transmit_pm_data() {
  PM25_AQI_Data data;
  
  if (! aqi.read(&data)) {
    Serial.println("Could not read from AQI");
    return; // Stop and try again on the next loop iteration
  }
  
  Serial.println("AQI reading success");
  
  // Print all values to Serial Monitor
  Serial.println(F("--- PM Data for CAN ---"));
  Serial.print(F("PM 1.0 (env): ")); Serial.println(data.pm10_env);
  Serial.print(F("PM 2.5 (env): ")); Serial.println(data.pm25_env);
  Serial.print(F("PM 10 (env): ")); Serial.println(data.pm100_env);
  Serial.print(F("PM2.5 AQI US: ")); Serial.println(data.aqi_pm25_us);
  Serial.print(F("PM10 AQI US: ")); Serial.println(data.aqi_pm100_us);
  Serial.println(F("-----------------------"));

  // --- MESSAGE 1: Environmental Concentrations (3 values * 2 bytes = 6 bytes) ---
  // Contains: pm10_env, pm25_env, pm100_env
  uint8_t concentration_data[6];
  
  // Byte 0-1: PM 1.0 (High Byte, Low Byte)
  concentration_data[0] = (uint8_t)(data.pm10_env >> 8);
  concentration_data[1] = (uint8_t)(data.pm10_env & 0xFF);
  
  // Byte 2-3: PM 2.5
  concentration_data[2] = (uint8_t)(data.pm25_env >> 8);
  concentration_data[3] = (uint8_t)(data.pm25_env & 0xFF);
  
  // Byte 4-5: PM 10
  concentration_data[4] = (uint8_t)(data.pm100_env >> 8);
  concentration_data[5] = (uint8_t)(data.pm100_env & 0xFF);

  transmit_can_message(CAN_ID_PM_CONCENTRATION, 6, concentration_data);

  // --- MESSAGE 2: AQI Values (2 values * 2 bytes = 4 bytes) ---
  // Contains: aqi_pm25_us, aqi_pm100_us
  uint8_t aqi_data[4];
  
  // Byte 0-1: PM 2.5 AQI
  aqi_data[0] = (uint8_t)(data.aqi_pm25_us >> 8);
  aqi_data[1] = (uint8_t)(data.aqi_pm25_us & 0xFF);
  
  // Byte 2-3: PM 10 AQI
  aqi_data[2] = (uint8_t)(data.aqi_pm100_us >> 8);
  aqi_data[3] = (uint8_t)(data.aqi_pm100_us & 0xFF);

  transmit_can_message(CAN_ID_PM_AQI_US, 4, aqi_data);

  // Add a small delay between the two message transmissions to ensure they don't block
  // the CAN buffer if the bus is heavily loaded.
  delay(10); 
}


// #include <Arduino.h>
// #include "driver/twai.h"
// // #include <Adafruit_PM25AQI.h> // No longer needed

// // --- CAN BUS CONFIGURATION ---
// #define CAN_TX_PIN 5
// #define CAN_RX_PIN 4
// #define TRANSMIT_INTERVAL_MS 1000

// // Define two distinct CAN IDs for the data streams
// #define CAN_ID_PM_CONCENTRATION 0x100 // ID for PM 1.0, 2.5, 10 environmental concentrations
// #define CAN_ID_PM_AQI_US        0x101 // ID for PM 2.5 AQI and PM 10 AQI values

// // --- MOCK DATA VALUES ---
// // Using 16-bit values (uint16_t) to simulate the sensor data structure
// const uint16_t MOCK_PM1_ENV = 5;      // Mock PM 1.0 concentration
// const uint16_t MOCK_PM25_ENV = 12;    // Mock PM 2.5 concentration
// const uint16_t MOCK_PM10_ENV = 25;    // Mock PM 10 concentration
// const uint16_t MOCK_PM25_AQI = 50;    // Mock PM 2.5 AQI value (Good/Moderate)
// const uint16_t MOCK_PM10_AQI = 20;    // Mock PM 10 AQI value

// // --- GLOBAL VARIABLES ---
// // Adafruit_PM25AQI aqi = Adafruit_PM25AQI(); // No longer needed
// unsigned long previousMillis = 0; // store last time message was sent

// // --- FUNCTION PROTOTYPES ---
// void setup_can();
// // void setup_pm_sensor(); // No longer needed
// void transmit_mock_pm_data();

// // Helper function to transmit a single CAN message and report status
// esp_err_t transmit_can_message(uint32_t identifier, uint8_t data_len, uint8_t data[]);


// // =========================================================================
// // SETUP FUNCTION
// // =========================================================================
// void setup() {
//   Serial.begin(115200);
//   delay(1000);

//   // Initialize the CAN bus driver
//   setup_can(); 
  
//   // No PM sensor setup needed!
//   Serial.println("PM sensor setup skipped. Sending mock data.");
// }

// // =========================================================================
// // LOOP FUNCTION
// // =========================================================================
// void loop() {
//   unsigned long currentMillis = millis();

//   // Check if it's time to send a message (e.g., every 1000ms)
//   if (currentMillis - previousMillis >= TRANSMIT_INTERVAL_MS) {
//     previousMillis = currentMillis;

//     // Transmit the hardcoded mock data over CAN
//     transmit_mock_pm_data();
//   }
// }

// // =========================================================================
// // CONFIGURATION FUNCTIONS
// // =========================================================================

// void setup_can() {
//     Serial.println("Initializing TWAI (CAN)…");
//     twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
//     twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
//     twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

//     // Install driver
//     if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
//       Serial.println("CAN Driver installed");
//     } else {
//       Serial.println("Failed to install CAN driver");
//       // Consider an infinite loop or reset here if CAN is critical
//       return; 
//     }

//     // Start driver
//     if (twai_start() == ESP_OK) {
//       Serial.println("CAN Driver started");
//     } else {
//       Serial.println("Failed to start CAN driver");
//       // Consider an infinite loop or reset here if CAN is critical
//       return;
//     }
// }

// // =========================================================================
// // TRANSMISSION FUNCTIONS
// // =========================================================================

// // Helper function to transmit a single CAN message and report status
// esp_err_t transmit_can_message(uint32_t identifier, uint8_t data_len, uint8_t data[]) {
//   twai_message_t message;
//   message.identifier = identifier;
//   message.extd = 0; // Standard ID
//   message.data_length_code = data_len;
//   memcpy(message.data, data, data_len);

//   // Wait up to 100ms for a free transmission slot
//   esp_err_t status = twai_transmit(&message, pdMS_TO_TICKS(100)); 

//   if (status == ESP_OK) {
//       Serial.print("Message 0x");
//       Serial.print(identifier, HEX);
//       Serial.println(" queued for transmission");
//   } else {
//       Serial.print("Failed to queue message 0x");
//       Serial.print(identifier, HEX);
//       Serial.print(": ");
//       Serial.println(esp_err_to_name(status));
//   }
//   return status;
// }

// // Main function to package and send two mock CAN messages
// void transmit_mock_pm_data() {
  
//   Serial.println(F("--- MOCK PM Data for CAN ---"));
//   Serial.print(F("PM 1.0 (env): ")); Serial.println(MOCK_PM1_ENV);
//   Serial.print(F("PM 2.5 (env): ")); Serial.println(MOCK_PM25_ENV);
//   Serial.print(F("PM 10 (env): ")); Serial.println(MOCK_PM10_ENV);
//   Serial.print(F("PM2.5 AQI US: ")); Serial.println(MOCK_PM25_AQI);
//   Serial.print(F("PM10 AQI US: ")); Serial.println(MOCK_PM10_AQI);
//   Serial.println(F("-----------------------"));

//   // --- MESSAGE 1: Environmental Concentrations (3 values * 2 bytes = 6 bytes) ---
//   // Contains: pm10_env, pm25_env, pm100_env
//   uint8_t concentration_data[6];
  
//   // Byte 0-1: PM 1.0 (High Byte, Low Byte)
//   concentration_data[0] = (uint8_t)(MOCK_PM1_ENV >> 8);
//   concentration_data[1] = (uint8_t)(MOCK_PM1_ENV & 0xFF);
  
//   // Byte 2-3: PM 2.5
//   concentration_data[2] = (uint8_t)(MOCK_PM25_ENV >> 8);
//   concentration_data[3] = (uint8_t)(MOCK_PM25_ENV & 0xFF);
  
//   // Byte 4-5: PM 10
//   concentration_data[4] = (uint8_t)(MOCK_PM10_ENV >> 8);
//   concentration_data[5] = (uint8_t)(MOCK_PM10_ENV & 0xFF);

//   transmit_can_message(CAN_ID_PM_CONCENTRATION, 6, concentration_data);

//   // --- MESSAGE 2: AQI Values (2 values * 2 bytes = 4 bytes) ---
//   // Contains: aqi_pm25_us, aqi_pm100_us
//   uint8_t aqi_data[4];
  
//   // Byte 0-1: PM 2.5 AQI
//   aqi_data[0] = (uint8_t)(MOCK_PM25_AQI >> 8);
//   aqi_data[1] = (uint8_t)(MOCK_PM25_AQI & 0xFF);
  
//   // Byte 2-3: PM 10 AQI
//   aqi_data[2] = (uint8_t)(MOCK_PM10_AQI >> 8);
//   aqi_data[3] = (uint8_t)(MOCK_PM10_AQI & 0xFF);

//   transmit_can_message(CAN_ID_PM_AQI_US, 4, aqi_data);

//   // Add a small delay between the two message transmissions to ensure they don't block
//   // the CAN buffer if the bus is heavily loaded.
//   delay(10); 
// }