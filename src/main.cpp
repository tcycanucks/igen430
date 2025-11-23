#include <Arduino.h>
#include "driver/twai.h"
#include <Adafruit_PM25AQI.h> 

// --- CAN BUS CONFIGURATION ---
#define CAN_TX_PIN 5
#define CAN_RX_PIN 4
#define CAN_ID_PM25 0x100  // Custom CAN ID for PM2.5 data (256 in decimal)
#define TRANSMIT_INTERVAL_MS 1000

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
  // Wait for serial monitor to open
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("Adafruit PMSA003I Air Quality Sensor");
  
  // Change default Serial1 pins (TX=GPIO 10, RX=GPIO 9) to safer pins:
  // Serial.begin(BaudRate, Configuration, RX_Pin, TX_Pin)
  Serial1.begin(9600, SERIAL_8N1, 16, 17); // Example: TX on GPIO 17, RX on GPIO 16

  // Wait three seconds for sensor to boot up!
  delay(3000);

  // Connectivity:
  // 2. Only attempt UART connection
    if (! aqi.begin_UART(&Serial1)) { 
      Serial.println("Could not find PM 2.5 sensor via UART!");
      while (1) delay(10);
    }
    Serial.println("PM25 found via UART!");
}

void loop() {
  PM25_AQI_Data data;
  
  if (! aqi.read(&data)) {
    Serial.println("Could not read from AQI");
    
    delay(500);  // try again in a bit!
    return;
  }
  Serial.println("AQI reading success");

  Serial.println(F("---------------------------------------"));
  Serial.println(F("Concentration Units (environmental)"));
  Serial.print(F("PM 1.0: ")); Serial.print(data.pm10_env);
  Serial.print(F("\t\tPM 2.5: ")); Serial.print(data.pm25_env);
  Serial.print(F("\t\tPM 10: ")); Serial.println(data.pm100_env);
  Serial.println(F("AQI"));
  Serial.print(F("PM2.5 AQI US: ")); Serial.print(data.aqi_pm25_us);
  Serial.print(F("\tPM10  AQI US: ")); Serial.println(data.aqi_pm100_us);
  Serial.println(F("---------------------------------------"));
  Serial.println();

  delay(1000);
}



