/*
 * services.c
 *
 *  Created on: Nov 25, 2024
 *      Author: Peter Alpajaro
 */


// This file contains the sensor init and sensor state machines

// Includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sensor.h"
#include "gatt_db.h"
#include "bluenrg1_gap.h"
#include "bluenrg1_gap_aci.h"
#include "bluenrg1_hci_le.h"
#include "hci_const.h"
#include "bluenrg1_gatt_aci.h"

// Private Variables
extern uint8_t bdaddr[BDADDR_SIZE];
extern uint8_t bnrg_expansion_board;
extern uint16_t NumberCharHandle;

__IO uint8_t set_connectable = 1;
__IO uint16_t connection_handle = 0;
__IO uint8_t notification_enabled = FALSE;
__IO uint8_t connected = FALSE;
__IO uint8_t pairing = FALSE;
__IO uint8_t paired = FALSE;

// ?
volatile uint8_t request_free_fall_notify = FALSE;

// Private function prototypes
void GAP_DisconnnectionComplete_CB(void);
void GAP_ConnectionComplete_CB(uint8_t addr[6], uint16_t handle);

// Private functions

/**
 *
 * @brief	Set_DeviceConnectable
 * @note	Puts the devie in a connectable mode
 * @param	None
 * @retval None
 */
void Set_DeviceConnectable(void)
{
	uint8_t ret;
	uint8_t local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME, SENSOR_DEMO_NAME};

	uint8_t manuf_data[26] = {
		2, 0x0A, 0x00, // 0dBm transmission power
		8, 0x09, SENSOR_DEMO_NAME, // Complete name :TODO find and replace.
		13, 0xFF, 0x01, // SKD Version?
		0x80,
		0x00,
		0xF4,
		0x00,
		0x00,
		bdaddr[5], // BLE MAC START -MSB FIRST-
		bdaddr[4],
		bdaddr[3],
		bdaddr[2],
		bdaddr[1],
		bdaddr[0] // BLE MAC STOP
	};

	// Sensor fusion?
	manuf_data[18] |= 0x01;

	hci_le_set_scan_response_data(0, NULL);

	PRINT_DBG("Set General Discoverable Mode.\r\n");

	ret = aci_gap_set_discoverable(ADV_DATA_TYPE,
									ADV_INTERV_MIN, ADV_INTERV_MAX,
									PUBLIC_ADDR,
									NO_WHITE_LIST_USE,
									sizeof(local_name), local_name, 0, NULL, 0, 0);

	aci_gap_update_adv_data(26, manuf_data);

	if (ret != BLE_STATUS_SUCCESS) {
		PRINT_DBG("aci_gap_set_discoverable() failed: 0x%02x\r\n", ret);
	} else {
		PRINT_DBG("aci_gap_set_discoverable() success!\r\n");
	}

}

/*
 *
 * @brief 	Callback processing the ACI events
 * @note	Inside this function each event must be identified and correctly parsed
 * @param 	void* pointer to the ACI packet
 * @retval	none
 *
 */
void APP_UserEvtRx(void *pData)
{
	uint32_t i;

	  hci_spi_pckt *hci_pckt = (hci_spi_pckt *)pData;

	  if(hci_pckt->type == HCI_EVENT_PKT)
	  {
	    hci_event_pckt *event_pckt = (hci_event_pckt*)hci_pckt->data;

	    if(event_pckt->evt == EVT_LE_META_EVENT)
	    {
	      evt_le_meta_event *evt = (void *)event_pckt->data;

	      for (i = 0; i < (sizeof(hci_le_meta_events_table)/sizeof(hci_le_meta_events_table_type)); i++)
	      {
	        if (evt->subevent == hci_le_meta_events_table[i].evt_code)
	        {
	          hci_le_meta_events_table[i].process((void *)evt->data);
	        }
	      }
	    }
	    else if(event_pckt->evt == EVT_VENDOR)
	    {
	      evt_blue_aci *blue_evt = (void*)event_pckt->data;

	      for (i = 0; i < (sizeof(hci_vendor_specific_events_table)/sizeof(hci_vendor_specific_events_table_type)); i++)
	      {
	        if (blue_evt->ecode == hci_vendor_specific_events_table[i].evt_code)
	        {
	          hci_vendor_specific_events_table[i].process((void *)blue_evt->data);
	        }
	      }
	    }
	    else
	    {
	      for (i = 0; i < (sizeof(hci_events_table)/sizeof(hci_events_table_type)); i++)
	      {
	        if (event_pckt->evt == hci_events_table[i].evt_code)
	        {
	          hci_events_table[i].process((void *)event_pckt->data);
	        }
	      }
	    }
	  }
}
