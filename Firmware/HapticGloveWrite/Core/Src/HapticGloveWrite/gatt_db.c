/*
 * gatt_db.c
 *
 *  Created on: Nov 25, 2024
 *      Author: Peter Alpajaro
 */


/**
  ******************************************************************************
  * @file    App/gatt_db.c
  * @author  SRA Application Team
  * @brief   Functions to build GATT DB and handle GATT events.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gatt_db.h"
#include "bluenrg1_aci.h"
#include "bluenrg1_hci_le.h"
#include "bluenrg1_gatt_aci.h"

#include "bluenrg_init.h"
#include "sensor.h"
#include "main.h"

/* Private macros ------------------------------------------------------------*/
/** @brief Macro that stores Value into a buffer in Little Endian Format (2 bytes)*/
#define HOST_TO_LE_16(buf, val)    ( ((buf)[0] =  (uint8_t) (val)    ) , \
                                   ((buf)[1] =  (uint8_t) (val>>8) ) )

/** @brief Macro that stores Value into a buffer in Little Endian Format (4 bytes) */
#define HOST_TO_LE_32(buf, val)    ( ((buf)[0] =  (uint8_t) (val)     ) , \
                                   ((buf)[1] =  (uint8_t) (val>>8)  ) , \
                                   ((buf)[2] =  (uint8_t) (val>>16) ) , \
                                   ((buf)[3] =  (uint8_t) (val>>24) ) )

#define COPY_UUID_128(uuid_struct, uuid_15, uuid_14, uuid_13, uuid_12, uuid_11, uuid_10, uuid_9, uuid_8, uuid_7, uuid_6, uuid_5, uuid_4, uuid_3, uuid_2, uuid_1, uuid_0) \
do {\
    uuid_struct[0] = uuid_0; uuid_struct[1] = uuid_1; uuid_struct[2] = uuid_2; uuid_struct[3] = uuid_3; \
        uuid_struct[4] = uuid_4; uuid_struct[5] = uuid_5; uuid_struct[6] = uuid_6; uuid_struct[7] = uuid_7; \
            uuid_struct[8] = uuid_8; uuid_struct[9] = uuid_9; uuid_struct[10] = uuid_10; uuid_struct[11] = uuid_11; \
                uuid_struct[12] = uuid_12; uuid_struct[13] = uuid_13; uuid_struct[14] = uuid_14; uuid_struct[15] = uuid_15; \
}while(0)

/* Hardware Characteristics Service */
//TODO: CHANGE THE UUID TO WHAT YOU WANT
#define COPY_HW_SENS_W2ST_SERVICE_UUID(uuid_struct)    COPY_UUID_128(uuid_struct,0x00,0x00,0x00,0x00,0x00,0x01,0x11,0xe1,0x9a,0xb4,0x00,0x02,0xa5,0xd5,0xc5,0x1b)
#define COPY_ENVIRONMENTAL_W2ST_CHAR_UUID(uuid_struct) COPY_UUID_128(uuid_struct,0x00,0x00,0x00,0x00,0x00,0x01,0x11,0xe1,0xac,0x36,0x00,0x02,0xa5,0xd5,0xc5,0x1b)
#define COPY_ACC_GYRO_MAG_W2ST_CHAR_UUID(uuid_struct)  COPY_UUID_128(uuid_struct,0x00,0xE0,0x00,0x00,0x00,0x01,0x11,0xe1,0xac,0x36,0x00,0x02,0xa5,0xd5,0xc5,0x1b)
/* Software Characteristics Service */
#define COPY_SW_SENS_W2ST_SERVICE_UUID(uuid_struct)    COPY_UUID_128(uuid_struct,0x00,0x00,0x00,0x00,0x00,0x02,0x11,0xe1,0x9a,0xb4,0x00,0x02,0xa5,0xd5,0xc5,0x1b)
#define COPY_QUATERNIONS_W2ST_CHAR_UUID(uuid_struct)   COPY_UUID_128(uuid_struct,0x00,0x00,0x01,0x00,0x00,0x01,0x11,0xe1,0xac,0x36,0x00,0x02,0xa5,0xd5,0xc5,0x1b)

// MARK: What we care about
#define COPY_GRID_W2ST_CHAR_UUID(uuid_struct) 			COPY_UUID_128(uuid_struct,0x00,0x00,0x00,0x01,0x00,0x01,0x11,0xe1,0xac,0x36,0x00,0x02,0xa5,0xd5,0xc5,0x1b)

uint16_t GridCharHandle;

/* Private variables ---------------------------------------------------------*/
uint16_t HWServW2STHandle, EnvironmentalCharHandle, AccGyroMagCharHandle;
uint16_t SWServW2STHandle, QuaternionsCharHandle;
float grid[4];
//static volatile uint8_t notifiation_enabled = FALSE;

/* UUIDS */
Service_UUID_t service_uuid;
Char_UUID_t char_uuid;

// TODO: CHANGE THESE
extern AxesRaw_t x_axes;
extern AxesRaw_t g_axes;
extern AxesRaw_t m_axes;
__IO uint8_t send_env;
__IO uint8_t send_mot;
__IO uint8_t send_quat;

extern __IO uint16_t connection_handle;
extern uint32_t start_time;

/* Private functions ---------------------------------------------------------*/
/**
 * @brief  Add the 'HW' service (and the Environmental and AccGyr characteristics).
 * @param  None
 * @retval tBleStatus Status
 */
tBleStatus Add_HWServW2ST_Service(void)
{
    tBleStatus ret;
    uint8_t uuid[16];

    // Add SW_SENS_W2ST service
    COPY_SW_SENS_W2ST_SERVICE_UUID(uuid);
    BLUENRG_memcpy(&service_uuid.Service_UUID_128, uuid, 16);
    ret = aci_gatt_add_service(UUID_TYPE_128, &service_uuid, PRIMARY_SERVICE,
                               1+(3*1), &SWServW2STHandle);
    if (ret != BLE_STATUS_SUCCESS) {
        return BLE_STATUS_ERROR;
    }

    // Add Grid characteristic
    COPY_GRID_W2ST_CHAR_UUID(uuid);
    BLUENRG_memcpy(&char_uuid.Char_UUID_128, uuid, 16);
    ret = aci_gatt_add_char(SWServW2STHandle, UUID_TYPE_128, &char_uuid,
                            2+4*4, // 2 bytes for timestamp, 4 floats (4 bytes each)
                            CHAR_PROP_NOTIFY | CHAR_PROP_READ | CHAR_PROP_WRITE,
                            ATTR_PERMISSION_NONE,
                            GATT_NOTIFY_ATTRIBUTE_WRITE,
                            16, 0, &GridCharHandle);
    if (ret != BLE_STATUS_SUCCESS) {
        return BLE_STATUS_ERROR;
    }

    return BLE_STATUS_SUCCESS;
}



tBleStatus Grid_Update(float grid[2][2])
{
	PRINT_DBG("Updating Grid Values\r\n");
	PRINT_DBG("HWServW2STHandle: 0x%04X, GridCharHandle: 0x%04X\r\n", SWServW2STHandle, GridCharHandle);

    tBleStatus ret;
    uint8_t buff[2+4*4];

    HOST_TO_LE_16(buff, HAL_GetTick()>>3);

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            uint32_t temp;
            memcpy(&temp, &grid[i][j], sizeof(float));
            HOST_TO_LE_32(buff + 2 + (i*2+j)*4, temp);
        }
    }

    ret = aci_gatt_update_char_value(SWServW2STHandle, GridCharHandle,
                                     0, 2+4*4, buff);
    if (ret != BLE_STATUS_SUCCESS) {
        PRINT_DBG("Error while updating Grid characteristic: 0x%02X\r\n", ret);
        return BLE_STATUS_ERROR;
    }

    return BLE_STATUS_SUCCESS;
}


// TODO: MODIFY THIS INTO UPDATING THE CAMERA VALUE.


/**
 * @brief  Update the sensor value
 *
 * @param  Handle of the characteristic to update
 * @retval None
 */
void Read_Request_CB(uint16_t handle)
{
  tBleStatus ret;

  if(handle == AccGyroMagCharHandle + 1)
  {
    //Acc_Update(&x_axes, &g_axes, &m_axes);
  }
  else if (handle == EnvironmentalCharHandle + 1)
  {
    float data_t, data_p;
    data_t = 27.0 + ((uint64_t)rand()*5)/RAND_MAX; //T sensor emulation
    data_p = 1000.0 + ((uint64_t)rand()*100)/RAND_MAX; //P sensor emulation
    //Environmental_Update((int32_t)(data_p *100), (int16_t)(data_t * 10));
  }

  if(connection_handle !=0)
  {
    ret = aci_gatt_allow_read(connection_handle);
    if (ret != BLE_STATUS_SUCCESS)
    {
      PRINT_DBG("aci_gatt_allow_read() failed: 0x%02x\r\n", ret);
    }
  }
}

// Assumes little endian
void printBits(size_t const size, uint8_t* ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
        printf(" ");
    }
    puts("");
}

int compare_floats(float a, float b, float epsilon) {
    return (a - b) > epsilon;
}

/**
 * @brief  This function is called when there is a change on the gatt attribute.
 *         With this function it's possible to understand if one application
 *         is subscribed to the one service or not.
 *
 * @param  uint16_t att_handle Handle of the attribute
 * @param  uint8_t  *att_data attribute data
 * @param  uint8_t  data_length length of the data
 * @retval None
 */
void Attribute_Modified_Request_CB(uint16_t Connection_Handle,
									uint16_t attr_handle,
									uint16_t Offset,
									uint8_t data_length,
									uint8_t *att_data)
{
	float grid[4];

	PRINT_DBG("GRID_CHAR_HANDLE: 0x%04X\r\n", GridCharHandle);
	if (attr_handle == GridCharHandle + 1) { // Replace GridCharHandle with your characteristic handle
	        PRINT_DBG("Characteristic written: Handle=0x%04X, Data Length=%d\r\n",
	                  attr_handle, data_length);

	        // Timestamp
	        uint16_t timestamp = att_data[0] | (att_data[1] << 8);
	        PRINT_DBG("Timestamp: %u\r\n", timestamp);

	        for (int i = 0; i < 4; ++i) {
	        	float value;
	        	memcpy(&value, att_data + 2 + (i*4), 4);
	        	PRINT_DBG("Float %d: %f (Raw: 0x%08X)\r\n", i, value, *(uint32_t*)(att_data + 2 + (i * 4)));

	        	// Convert 4 bytes to float
	        					int index = 2 + (i * 4);

	        	                uint32_t temp = 0;
	        	                memcpy(&temp, att_data + 2 + (i*4), 4);

	        	               union {
	        	                	uint32_t i;
	        	                	float f;
	        	                } converter;
	        	               converter.i = temp;


	        	                // Use memcpy to avoid strict aliasing violations
	        	                //memcpy(&value, &temp, sizeof(float));


	        	               	grid[i] = converter.f;
	        	                PRINT_DBG("Grid[%d]: %f\r\n", i, grid[i]);

	        }

	        // Hex dump
	        PRINT_DBG("Full hex dump:\r\n");
	        for (int i = 0; i < data_length; i++) {
	        	PRINT_DBG("%02X ", att_data[i]);
	        	if ((i + 1) % 8 == 0) PRINT_DBG("\r\n");


	        }
	        PRINT_DBG("\r\n");

	        // Process the data (assuming it's a float, adjust as needed)
	        printBits(18, att_data);

	        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, 1);

	        if (compare_floats((float) 0.5, grid[0], 0.00001)) {

	        	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, 0);
	        } else {
	        	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, 1);
	        }

	        // Handle the new data as needed
	        change_pwm_pulse_2(&htim2, TIM_CHANNEL_3, (uint32_t) roundf(grid[0] * 24));
	        change_pwm_pulse_2(&htim2, TIM_CHANNEL_4, (uint32_t) roundf(grid[1] * 24));
	        change_pwm_pulse(&htim16, TIM_CHANNEL_1, (uint16_t) roundf(grid[2] * 24));
	        change_pwm_pulse(&htim1, TIM_CHANNEL_4, (uint16_t) roundf(grid[3] * 24));
	    } else {
	        PRINT_DBG("Attribute modification for unknown handle: 0x%04X\r\n", attr_handle);
	    }
}

