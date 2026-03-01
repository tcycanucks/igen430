#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdint.h>

void setup_can();
void setup_pm_sensor();
void transmit_can_message(uint32_t identifier, uint8_t data_len, uint8_t data[]);

#endif // FUNCTIONS_H