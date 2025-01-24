// File: bluenrg_init.c
// Author: Peter Alpajaro
// Written on November 25th 2024

// This file has functions called during startup of the device, and sets up the bluetooth interface.

// INCLUDES:

#include "bluenrg_init.h"

#include <stdlib.h>

#include "bluenrg1_aci.h"
#include "bluenrg1_hci_le.h"
#include "bluenrg1_events.h"
#include "hci_tl.h"
#include "bluenrg_utils.h"

#include "gatt_db.h"
#include "sensor.h"


// Do these need to change? / Is the authentication hard coded into the swift code?
#define SECURE_PAIRING (0)
#define PERIPHERAL_PASS_KEY (123456)

// TODO: Verify that this is not being used.
//#define USE_BUTTON(0)

// Private variables

extern __IO uint16_t connection_handle;

// These act as booleans to manage our connection status.
extern volatile uint8_t set_connectable;
extern volatile uint8_t connected;
extern volatile uint8_t pairing;
extern volatile uint8_t paired;

// This is our MAC address
uint8_t bdaddr[BDADDR_SIZE];

//static volatile user_button_init_state = 1; TODO: Ensure this is not being used
// static volatile uint8_t user_button_pressed = 0;
extern __IO uint8_t send_num;


// -- Private Function Declarations
static void User_Process(void);
static void User_Init(void);
static uint8_t Sensor_DeviceInit(void);
static void Set_Number(float* data);

/**
 *
 *	@brief initializes the blutooth
 *  @param None
 * 	@retval None
 *
 *
 **/
void MX_BlueNRG_2_Init(void) {

	uint8_t ret;

	User_Init();

	// user_button_init_state = BSP_PB_GetState(BUTTON_KEY);

	// This passes the callback that we made in sensor, which lets give it to this function, and it will call
	// whenever some event occurs.
	hci_init(APP_UserEvtRx, NULL);

	PRINT_DBG("\033[2J"); // Serial console clear screen
	PRINT_DBG("\033[H"); // Serial console cursor to home
	PRINT_DBG("Haptic Glove Grid Write Application\r\n");

	// Initializing the sensor device
	ret = Sensor_DeviceInit();
	if (ret != BLE_STATUS_SUCCESS)
	{
		// Turn on the LED
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, 1);
		while (1); // endless loop to stop program.

	}

	PRINT_DBG("BLE Stack Initialized and Device Configured\r\n");

}

// BlueNRG-2 background task
void MX_BlueNRG_2_Process(void)
{

	hci_user_evt_proc();
	User_Process();

}

/*
 * @brief Initialize User Process
 *
 * @param None
 * @retval None
 */
static void User_Init(void)
{

	// Null. Not required in this applciation

	// BSP_PB_Init()
	// BSP_LED_Init(LED2)
	// BSP_COM_Init(COM1)

}


/*
 *
 * @brief Initalize the device sensors
 * @param None
 * @retval None
 *
 */
uint8_t Sensor_DeviceInit(void)
{
	uint8_t ret;
	uint16_t service_handle, dev_name_char_handle, appearance_char_handle;
	uint8_t device_name[] = {SENSOR_DEMO_NAME};
	uint8_t hwVersion;
	uint16_t fwVersion;
	uint8_t bdaddr_len_out;
	uint8_t config_data_stored_static_random_address = 0x80; // This is an offset of a static random address stored in NVM?

	// Software reset of the device
	hci_reset();

	// The BlueNRG-2 required a minimum delay of 2000ms for device boot
	HAL_Delay(2000);

	// getting the bluenrg hw and firmware versions
	getBlueNRGVersion(&hwVersion, &fwVersion);

	PRINT_DBG("HWver %d\nFwver %d\r\n", hwVersion, fwVersion);

	ret = aci_hal_read_config_data(config_data_stored_static_random_address,
									&bdaddr_len_out, bdaddr);
	if (ret) {
		PRINT_DBG("Read Static Random address failed\r\n");
		//HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
	}

	if ((bdaddr[5] & 0xC0) != 0xC0) {
		PRINT_DBG("Static Random address not well formed\r\n");
		//HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
		while(1);
	}

	ret = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET,
									bdaddr_len_out,
									bdaddr);

	if (ret != BLE_STATUS_SUCCESS) {
		PRINT_DBG("aci_hal_write_config_data() Failed\r\n");
		//HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
	} else {
		PRINT_DBG("aci_hal_write_config_data() Success!\r\n");
	}


	// Set the TX power to -2 dBm
	aci_hal_set_tx_power_level(1, 4);
	if (ret != BLE_STATUS_SUCCESS) {
		//PRINT_DBG("aci_hal_set_tx_power_level() failed");
		//HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
	} else {
		PRINT_DBG("aci_hal_set_tx_power_level() Success!\r\n");
	}

	// GATT initialization
	ret = aci_gatt_init();
	if (ret != BLE_STATUS_SUCCESS) {
		PRINT_DBG("aci_gatt_init() failed\r\n");
		//HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
		return ret;
	} else {
		PRINT_DBG("aci_gatt_init() Success!\r\n");
	}

	// GAP Initialization
	ret = aci_gap_init(GAP_PERIPHERAL_ROLE, 0x00, 0x07, &service_handle,
														&dev_name_char_handle,
														&appearance_char_handle);

	if (ret != BLE_STATUS_SUCCESS) {
		PRINT_DBG("aci_gap_init() Failed\r\n");
		//HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
		return ret;
	} else {
		PRINT_DBG("aci_gap_init() Success!\r\n");
	}

	// Update the device name
	ret = aci_gatt_update_char_value(service_handle, dev_name_char_handle, 0,
											sizeof(device_name), device_name);

	if (ret != BLE_STATUS_SUCCESS) {
		PRINT_DBG("aci_gatt_update_char_value() Failed\r\n");
		//HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
		return ret;
	} else if (ret != BLE_STATUS_SUCCESS) {
		PRINT_DBG("aci_gap_update_char_value() Success!\r\n");
	}

	// Clear the security database, which ensures that each time the application is
	// executed the full bonding process is executed with passkey generation and setting.
//	ret = aci_gap_clear_security_db();
//	if (ret != BLE_STATUS_SUCCESS) {
//		PRINT_DBG("aci_gap_clear_security_db() failed.");
//		return ret;
//	} else {
//		PRINT_DBG("aci_gap_clear_security_db() Sucess!\r\n");
//	}

	  /*
	   * Set the I/O capability otherwise the Central device (e.g. the smartphone) will
	   * propose a PIN that will be accepted without any control.
	   */
//	  if (aci_gap_set_io_capability(IO_CAP_DISPLAY_ONLY)==BLE_STATUS_SUCCESS) {
//	    PRINT_DBG("I/O Capability Configurated\r\n");
//	  } else {
//	    PRINT_DBG("Error Setting I/O Capability\r\n");
//	  }

	  /* BLE Security v4.2 is supported: BLE stack FW version >= 2.x (new API prototype) */
	  ret = aci_gap_set_authentication_requirement(NO_BONDING,
	                                               MITM_PROTECTION_NOT_REQUIRED,
	                                               SC_IS_NOT_SUPPORTED,
	                                               KEYPRESS_IS_NOT_SUPPORTED,
	                                               7,
	                                               16,
	                                               USE_FIXED_PIN_FOR_PAIRING,
	                                               PERIPHERAL_PASS_KEY,
	                                               0x00); /* - 0x00: Public Identity Address
	                                                         - 0x01: Random (static) Identity Address */
	  if (ret != BLE_STATUS_SUCCESS) {
	    PRINT_DBG("aci_gap_set_authentication_requirement()failed: 0x%02x\r\n", ret);
	    //HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
	    return ret;
	  }
	  else {
	    PRINT_DBG("aci_gap_set_authentication_requirement() --> SUCCESS\r\n");
	  }

	  PRINT_DBG("BLE Stack Initialized with SUCCESS\r\n");

	  ret = Add_HWServW2ST_Service();
	  if (ret == BLE_STATUS_SUCCESS) {

	    PRINT_DBG("BlueNRG2 HW service added successfully.\r\n");
	  }
	  else {
	    PRINT_DBG("Error while adding BlueNRG2 HW service: 0x%02x\r\n", ret);
		  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
	    while(1);
	  }

//	  ret = Add_SWServW2ST_Service();
//	  if(ret == BLE_STATUS_SUCCESS) {
//		  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
//	     PRINT_DBG("BlueNRG2 SW service added successfully.\r\n");
//	  }
//	  else {
//	     PRINT_DBG("Error while adding BlueNRG2 HW service: 0x%02x\r\n", ret);
//	     while(1);
//	  }

	  return BLE_STATUS_SUCCESS;
}

/*
 *
 * @brief User Process
 *
 * @param None
 * @retval None
 */
static void User_Process(void)
{

	    float grid[2][2];

	    uint8_t ret = 0;

	    if (set_connectable) {
	    	PRINT_DBG("Setting device connectable!");
	        Set_DeviceConnectable();
	        set_connectable = FALSE;
	    }

	    if ((connected) && (!pairing))
	    {
	    	PRINT_DBG("STARTING PAIRING");
	        ret = aci_gap_slave_security_req(connection_handle);
	        if (ret != BLE_STATUS_SUCCESS) {
	            PRINT_DBG("aci_gap_slave_security_req() failed:0x%02x\r\n", ret);
	            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
	        }
	        else {
	            PRINT_DBG("aci_gap_slave_security_req --> SUCCESS\r\n");
	        }
	        pairing = TRUE;
	    }

	    if (paired) {
	        // Generate random values for the grid
	        for (int i = 0; i < 2; i++) {
	            for (int j = 0; j < 2; j++) {
	                grid[i][j] = ((float)rand() / RAND_MAX) * 100.0f; // Random float between 0 and 100
	            }
	        }

	        // Update the grid characteristic
	        //Grid_Update(grid);

	        // Toggle LED to indicate data sent
	        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);

	        // Wait before sending next update
	        HAL_Delay(1000);
	    }
}


/**
 * @brief  Get hardware and firmware version
 *
 * @param  Hardware version
 * @param  Firmware version
 * @retval Status
 */
uint8_t getBlueNRGVersion(uint8_t *hwVersion, uint16_t *fwVersion)
{
  uint8_t status;
  uint8_t hci_version, lmp_pal_version;
  uint16_t hci_revision, manufacturer_name, lmp_pal_subversion;

  status = hci_read_local_version_information(&hci_version, &hci_revision, &lmp_pal_version,
                                              &manufacturer_name, &lmp_pal_subversion);

  if (status == BLE_STATUS_SUCCESS) {
    *hwVersion = hci_revision >> 8;
    *fwVersion = (hci_revision & 0xFF) << 8;              // Major Version Number
    *fwVersion |= ((lmp_pal_subversion >> 4) & 0xF) << 4; // Minor Version Number
    *fwVersion |= lmp_pal_subversion & 0xF;               // Patch Version Number
  }
  return status;
}

/**
 * @brief  This event occurs when a connection is established
 * @param  Status Connection status
 * @param  Connection_Handle Handle identifying the connection
 * @param  Role Master/Slave role of the device
 * @param  Peer_Address_Type Address type of the peer
 * @param  Peer_Address Address of the peer
 * @param  Conn_Interval Connection interval used
 * @param  Conn_Latency Connection latency
 * @param  Supervision_Timeout Supervision timeout
 */
void hci_le_connection_complete_event(uint8_t Status,
        uint16_t Connection_Handle,
        uint8_t Role,
        uint8_t Peer_Address_Type,
        uint8_t Peer_Address[6],
        uint16_t Conn_Interval,
        uint16_t Conn_Latency,
        uint16_t Supervision_Timeout,
        uint8_t Master_Clock_Accuracy)
{
    if (Status == 0x00) { // Success
        connection_handle = Connection_Handle;
        connected = TRUE;
        set_connectable = FALSE;

        PRINT_DBG("Connected to device: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                  Peer_Address[5], Peer_Address[4], Peer_Address[3],
                  Peer_Address[2], Peer_Address[1], Peer_Address[0]);
        PRINT_DBG("Connection Interval: %d\r\n", Conn_Interval);
        PRINT_DBG("Supervision Timeout: %d\r\n", Supervision_Timeout);
    } else {
        PRINT_DBG("Connection failed with status: 0x%02X\r\n", Status);
    }
}

/**
 * @brief  This event occurs when a connection is terminated
 *
 * @param  See file bluenrg1_events.h
 * @retval See file bluenrg1_events.h
 */
void hci_disconnection_complete_event(uint8_t Status,
                                      uint16_t Connection_Handle,
                                      uint8_t Reason)
{
  connected = FALSE;
  pairing = FALSE;
  paired = FALSE;

  /* Make the device connectable again */
  set_connectable = TRUE;
  connection_handle = 0;
  PRINT_DBG("Disconnected (0x%02x)\r\n", Reason);

  // Turn on LED upon disconnect
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, 1);
}

/**
 * @brief  This event is given when a read request is received
 *         by the server from the client
 * @param  See file bluenrg1_events.h
 * @retval See file bluenrg1_events.h
 */
void aci_gatt_read_permit_req_event(uint16_t Connection_Handle,
                                    uint16_t Attribute_Handle,
                                    uint16_t Offset)
{
  Read_Request_CB(Attribute_Handle);
}

/**
 * @brief  This event is given when an attribute changes his value
 * @param  See file bluenrg1_events.h
 * @retval See file bluenrg1_events.h
 */
void aci_gatt_attribute_modified_event(uint16_t Connection_Handle,
                                       uint16_t Attribute_Handle,
                                       uint16_t Offset,
                                       uint16_t Attr_Data_Length,
                                       uint8_t Attr_Data[])
{
  Attribute_Modified_Request_CB(Connection_Handle, Attribute_Handle, Offset, Attr_Data_Length, Attr_Data);
  PRINT_DBG("" + Offset);
}

/**
 * @brief  This event is generated by the Security manager to the application
 *         when a passkey is required for pairing.
 *         When this event is received, the application has to respond with the
 *         aci_gap_pass_key_resp command.
 * @param  See file bluenrg1_events.h
 * @retval See file bluenrg1_events.h
 */
void aci_gap_pass_key_req_event(uint16_t Connection_Handle)
{
  uint8_t ret;

  ret = aci_gap_pass_key_resp(connection_handle, PERIPHERAL_PASS_KEY);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINT_DBG("aci_gap_pass_key_resp failed:0x%02x\r\n", ret);
  } else {
    PRINT_DBG("aci_gap_pass_key_resp OK\r\n");
  }
}

/**
 * @brief  This event is generated when the pairing process has completed successfully or a pairing
 *         procedure timeout has occurred or the pairing has failed. This is to notify the application that
 *         we have paired with a remote device so that it can take further actions or to notify that a
 *         timeout has occurred so that the upper layer can decide to disconnect the link.
 * @param  See file bluenrg1_events.h
 * @retval See file bluenrg1_events.h
 */
void aci_gap_pairing_complete_event(uint16_t connection_handle, uint8_t status, uint8_t reason)
{
  if (status == 0x02) { /* Pairing Failed */
    PRINT_DBG("aci_gap_pairing_complete_event failed:0x%02x with reason 0x%02x\r\n", status, reason);
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
  }
  else {
    paired = TRUE;
    PRINT_DBG("aci_gap_pairing_complete_event with status 0x%02x\r\n", status);
    //HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
    HAL_Delay(1000);
    //HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
  }
}
