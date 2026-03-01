#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// --- PIN CONFIGURATION ---
#define CAN_TX_PIN 5
#define CAN_RX_PIN 4
#define PM_RX_PIN 26 // Connects to PMS5003 TX
#define PM_TX_PIN 27 // Connects to PMS5003 RX

#include "Arduino.h"
#include <driver/twai.h>
#include <stdint.h>


#endif // PIN_CONFIG_H