#include <Arduino.h>
#include <Wire.h>
#include <DFRobot_AHT20.h>
#include <DFRobot_ENS160.h>

DFRobot_AHT20 aht21;
// We'll try address 0x53 first (Standard for this combo board)
DFRobot_ENS160_I2C ens160(&Wire, 0x53); 

bool ens160_alive = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n--- Testing 'Slow Mode' Fix ---");

  // --- THE FIX ---
  // We manually start Wire and force the clock speed to 100,000Hz (100kHz)
  Wire.begin(21, 22); 
  Wire.setClock(100000); 
  delay(100);

  // 1. Initialize AHT21
  if (aht21.begin() == 0) {
    Serial.println("AHT21: ONLINE");
  } else {
    Serial.println("AHT21: FAILED");
  }

  // 2. Initialize ENS160
  // If 0x53 fails, the code will try 0x52 automatically
  if (ens160.begin() == NO_ERR) {
    Serial.println("ENS160: ONLINE (Found at 0x53)");
    ens160_alive = true;
  } else {
    // Try the alternative address just in case
    ens160 = DFRobot_ENS160_I2C(&Wire, 0x52);
    if (ens160.begin() == NO_ERR) {
      Serial.println("ENS160: ONLINE (Found at 0x52)");
      ens160_alive = true;
    } else {
      Serial.println("ENS160: STILL NOT FOUND - Logic levels might be too low.");
      ens160_alive = false;
    }
  }

  if (ens160_alive) {
    ens160.setPWRMode(ENS160_STANDARD_MODE);
  }
}

void loop() {
  if (aht21.startMeasurementReady(true)) {
    float t = aht21.getTemperature_C();
    float h = aht21.getHumidity_RH();
    Serial.print("Temp: "); Serial.print(t); 
    Serial.print("C | Humid: "); Serial.print(h); Serial.println("%");

    if (ens160_alive) {
      ens160.setTempAndHum(t, h); // Give ENS160 data for better accuracy
      Serial.print("AQI: "); Serial.print(ens160.getAQI());
      Serial.print(" | TVOC: "); Serial.print(ens160.getTVOC());
      Serial.print("ppb | CO2: "); Serial.println(ens160.getECO2());
    }
  }
  
  Serial.println("--------------------------------");
  delay(2000);
}