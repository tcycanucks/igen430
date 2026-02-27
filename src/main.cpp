#include <Arduino.h>
#include <Wire.h>
#include "driver/twai.h"
#include <DFRobot_AHT20.h>
#include <DFRobot_ENS160.h>
#include <Adafruit_PM25AQI.h>

// --- PIN CONFIGURATION ---
#define CAN_TX_PIN 5
#define CAN_RX_PIN 4
#define PM_RX_PIN 26 // Connects to PMS5003 TX
#define PM_TX_PIN 27 // Connects to PMS5003 RX

// --- CAN CONFIGURATION ---
#define CAN_ID_PM_CONCENTRATION 0x100
#define CAN_ID_PM_AQI_US        0x101
#define TRANSMIT_INTERVAL_MS 2000 // Update every 2 seconds

// --- GLOBAL OBJECTS ---
DFRobot_AHT20 aht21;
DFRobot_ENS160_I2C ens160(&Wire, 0x53);
Adafruit_PM25AQI aqi = Adafruit_PM25AQI();

bool ens160_alive = false;
unsigned long previousMillis = 0;

// --- PROTOTYPES ---
void setup_can();
void setup_pm_sensor();
void transmit_can_message(uint32_t identifier, uint8_t data_len, uint8_t data[]);

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n--- Integrated Air Quality Station ---");

  // 1. Start I2C (Slow Mode for ENS160 stability)
  Wire.begin(21, 22); 
  Wire.setClock(100000); 
  delay(100);

  // 2. Initialize Gas/Temp Sensors
  if (aht21.begin() == 0) Serial.println("AHT21: ONLINE");
  
  if (ens160.begin() == NO_ERR) {
    Serial.println("ENS160: ONLINE (0x53)");
    ens160_alive = true;
  } else {
    ens160 = DFRobot_ENS160_I2C(&Wire, 0x52);
    if (ens160.begin() == NO_ERR) {
      Serial.println("ENS160: ONLINE (0x52)");
      ens160_alive = true;
    }
  }
  if (ens160_alive) ens160.setPWRMode(ENS160_STANDARD_MODE);

  // 3. Initialize PM Sensor & CAN
  setup_pm_sensor();
  setup_can();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= TRANSMIT_INTERVAL_MS) {
    previousMillis = currentMillis;
    
    Serial.println("\n--- NEW SENSOR READING ---");

    // --- STEP 1: READ TEMP/HUMIDITY ---
    float t = 0, h = 0;
    if (aht21.startMeasurementReady(true)) {
      t = aht21.getTemperature_C();
      h = aht21.getHumidity_RH();
      Serial.printf("Temp: %.2fC | Humid: %.2f%%\n", t, h);
    }

    // --- STEP 2: READ GAS (ENS160) ---
    if (ens160_alive) {
      ens160.setTempAndHum(t, h);
      Serial.printf("AQI: %d | TVOC: %d ppb | eCO2: %d ppm\n", 
                    ens160.getAQI(), ens160.getTVOC(), ens160.getECO2());
    }

    // --- STEP 3: READ PM SENSOR & SEND CAN ---
    PM25_AQI_Data pm_data;
    if (aqi.read(&pm_data)) {
      Serial.printf("PM2.5: %d | PM10: %d | AQI: %d\n", 
                    pm_data.pm25_env, pm_data.pm100_env, pm_data.aqi_pm25_us);
      
      // Pack and Transmit CAN Messages
      uint8_t conc_payload[6] = {
        (uint8_t)(pm_data.pm10_env >> 8), (uint8_t)(pm_data.pm10_env & 0xFF),
        (uint8_t)(pm_data.pm25_env >> 8), (uint8_t)(pm_data.pm25_env & 0xFF),
        (uint8_t)(pm_data.pm100_env >> 8), (uint8_t)(pm_data.pm100_env & 0xFF)
      };
      transmit_can_message(CAN_ID_PM_CONCENTRATION, 6, conc_payload);
    } else {
      Serial.println("PM Sensor: Read Failed");
    }
    Serial.println("--------------------------");
  }
}

// --- HELPER FUNCTIONS ---

void setup_can() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK && twai_start() == ESP_OK) {
      Serial.println("CAN: ONLINE");
    } else {
      Serial.println("CAN: FAILED");
    }
}

void setup_pm_sensor() {
  Serial1.begin(9600, SERIAL_8N1, PM_RX_PIN, PM_TX_PIN); 
  delay(1000);
  if (aqi.begin_UART(&Serial1)) { 
    Serial.println("PM Sensor: ONLINE");
  } else {
    Serial.println("PM Sensor: NOT FOUND");
  }
}

void transmit_can_message(uint32_t identifier, uint8_t data_len, uint8_t data[]) {
  twai_message_t message;
  message.identifier = identifier;
  message.extd = 0;
  message.data_length_code = data_len;
  memcpy(message.data, data, data_len);
  twai_transmit(&message, pdMS_TO_TICKS(100));
}