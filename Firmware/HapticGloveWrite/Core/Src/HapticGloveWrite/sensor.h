/*
 * sensor.h
 *
 *  Created on: Nov 25, 2024
 *      Author: Peter Alpajaro
 */

#ifndef SRC_HAPTICGLOVEWRITE_SENSOR_H_
#define SRC_HAPTICGLOVEWRITE_SENSOR_H_

// Includes
#include <stdint.h>

#define SENSOR_DEMO_NAME 'H','a','p','t','i','c',' '
#define BDADDR_SIZE 6

void Set_DeviceConnectable(void);
void APP_UserEvtRx(void *pData);

extern volatile uint8_t notification_enabled;

#endif /* SRC_HAPTICGLOVEWRITE_SENSOR_H_ */
