/*
 * gatt_db.h
 *
 *  Created on: Nov 25, 2024
 *      Author: Peter Alpajaro
 */

#ifndef SRC_HAPTICGLOVEWRITE_GATT_DB_H_
#define SRC_HAPTICGLOVEWRITE_GATT_DB_H_

/* Includes ------------------------------------------------------------------*/
#include "hci.h"

/* Exported defines ----------------------------------------------------------*/
#define X_OFFSET 200
#define Y_OFFSET 50
#define Z_OFFSET 1000

/**
 * @brief Number of application services
 */
#define NUMBER_OF_APPLICATION_SERVICES (2)

/**
 * @brief Define How Many quaterions you want to transmit (from 1 to 3)
 *        In this sample application use only 1
 */
#define SEND_N_QUATERNIONS 1

/* Exported typedef ----------------------------------------------------------*/
/**
 * @brief Structure containing acceleration value of each axis.
 */
typedef struct {
  int32_t AXIS_X;
  int32_t AXIS_Y;
  int32_t AXIS_Z;
} AxesRaw_t;

enum {
  ACCELERATION_SERVICE_INDEX = 0,
  ENVIRONMENTAL_SERVICE_INDEX = 1
};

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim16;
extern TIM_HandleTypeDef htim1;

/* Exported function prototypes ----------------------------------------------*/
tBleStatus Add_HWServW2ST_Service(void);
tBleStatus Add_SWServW2ST_Service(void);
void Read_Request_CB(uint16_t handle);
void Attribute_Modified_Request_CB(uint16_t Connection_Handle, uint16_t attr_handle,
                                   uint16_t Offset, uint8_t data_length, uint8_t *att_data);

#endif /* SRC_HAPTICGLOVEWRITE_GATT_DB_H_ */
