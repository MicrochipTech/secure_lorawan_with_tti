/** m16946
* \file  main.c
*
* \brief LORAWAN Demo Application main file
*
*
* Copyright (c) 2018 Microchip Technology Inc. and its subsidiaries. 
*
* \asf_license_start
*
* \page License
*
* Subject to your compliance with these terms, you may use Microchip
* software and any derivatives exclusively with Microchip products. 
* It is your responsibility to comply with third party license terms applicable 
* to your use of third party software (including open source software) that 
* may accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, 
* WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, 
* INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, 
* AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE 
* LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL 
* LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE 
* SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE 
* POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT 
* ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY 
* RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, 
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*
* \asf_license_stop
*
*/
/*
* Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
*/
 
/*** INCLUDE FILES ************************************************************/
#include <asf.h>
#include "system_low_power.h"
#include "radio_driver_hal.h"
#include "lorawan.h"
#include "sys.h"
#include "system_init.h"
#include "system_assert.h"
#include "aes_engine.h"
#include "sio2host.h"
#include "extint.h"
#include "sw_timer.h"
#include "edbg_eui.h"
#include "resources.h"
#include "temp_sensor.h"
#include "LED.h"
#ifdef CONF_PMM_ENABLE
#include "pmm.h"
#include  "conf_pmm.h"
#include "sleep_timer.h"
#include "sleep.h"
#endif
#include "conf_sio2host.h"
#if (ENABLE_PDS == 1)
#include "pds_interface.h"
#endif
#ifdef CRYPTO_DEV_ENABLED
#include "sal.h"

// m16946 added
#include "cryptoauthlib.h"
#include "conf_sal.h"
#endif

typedef enum {LAB1_APP = 1, LAB2_APP = 2} AppType_t ;
//#define CLONE 1

/*** SYMBOLIC CONSTANTS ********************************************************/
#define APP_DEBOUNCE_TIME						50		// Button debounce in ms
#define DEMO_CONF_DEFAULT_APP_SLEEP_TIME_MS     60000	// Sleep duration in ms
#define APP_TIMEOUT								15000	// App timeout value in ms
#define APP_PORT								10		// LoRaWAN Application Port

/*** GLOBAL VARIABLES & TYPE DEFINITIONS ***************************************/
AppType_t currentApp ;
// LEDs states
static uint8_t on = LON ;
static uint8_t off = LOFF ;
static uint8_t toggle = LTOGGLE ;

// Application Timer ID
uint8_t AppTimerID = 0xFF ;

// Button pressed
bool buttonPressed = false ;
uint16_t buttonCount = 0 ;

// LoRaWAN
bool uplinkFlag = false ;
uint8_t uplinkTimeout = 0 ;

bool downlinkReceived = false ;
uint8_t app_buf[3] = {0x00, 0x00, 0x00} ;
char g_payload[20] ;
char first_name[11] ;
uint8_t first_name_len = 10 ;

bool joined = false ;
LorawanSendReq_t lorawanSendReq ;

// OTAA join parameters
uint8_t demoDevEui[8] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x01 } ;
uint8_t demoAppEui[8] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 } ;
uint8_t demoAppKey[16] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 } ;

#ifdef CONF_PMM_ENABLE
bool deviceResetsForWakeup = false;
#endif

/*** LOCAL FUNCTION PROTOTYPES *************************************************/
static void app_init(void) ;
static void configure_extint(void) ;
static void configure_eic_callback(void) ;
static void extint_callback(void) ;
static void dev_eui_read(void) ;
static void print_array (uint8_t *array, uint8_t length) ;
SYSTEM_TaskStatus_t APP_TaskHandler(void) ;
static float convert_celsius_to_fahrenheit(float celsius_val) ;
static void send_uplink(TransmissionType_t type, uint8_t fport, void* data, uint8_t len) ;
void appTimer_callback(void) ;
void appdata_callback(void *appHandle, appCbParams_t *appdata) ;
void joindata_callback(StackRetStatus_t status) ;
void send_data(void) ;

int xtoi(char *c) ;
uint8_t Parser_HexAsciiToInt(uint16_t hexAsciiLen, char* pInHexAscii, uint8_t* pOutInt) ;
static void press_enter(void) ;

/*** LOCAL FUNCTION DEFINITIONS ************************************************/

#if (_DEBUG_ == 1)
static void assertHandler(SystemAssertLevel_t level, uint16_t code);
#endif /* #if (_DEBUG_ == 1) */

/*********************************************************************//**
 \brief      Uninitializes app resources before going to low power mode
*************************************************************************/
#ifdef CONF_PMM_ENABLE
static void app_resources_uninit(void);
#endif

static void print_reset_causes(void)
{
    enum system_reset_cause rcause = system_get_reset_cause();
    printf("Last reset cause: ");
    if(rcause & (1 << 6)) {
        printf("System Reset Request\r\n");
    }
    if(rcause & (1 << 5)) {
        printf("Watchdog Reset\r\n");
    }
    if(rcause & (1 << 4)) {
        printf("External Reset\r\n");
    }
    if(rcause & (1 << 2)) {
        printf("Brown Out 33 Detector Reset\r\n");
    }
    if(rcause & (1 << 1)) {
        printf("Brown Out 12 Detector Reset\r\n");
    }
    if(rcause & (1 << 0)) {
        printf("Power-On Reset\r\n");
    }
}

#if (_DEBUG_ == 1)
static void assertHandler(SystemAssertLevel_t level, uint16_t code)
{
    printf("\r\n%04x\r\n", code);
    (void)level;
}
#endif /* #if (_DEBUG_ == 1) */

/*** app_init *******************************************************************
 \brief      Function to initialize all the hardware and software modules
********************************************************************************/
static void app_init(void)
{
	system_init() ;
	delay_init() ;
	board_init() ;
	configure_extint() ;
	configure_eic_callback() ;
	INTERRUPT_GlobalInterruptEnable() ;
	sio2host_init() ;
#ifndef CRYPTO_DEV_ENABLED
#ifdef EDBG_EUI_READ
	/* Read DEV EUI from EDBG */
	dev_eui_read() ;
#endif
#endif
	// LoRaWAN Stack driver init
	HAL_RadioInit() ;
	SystemTimerInit() ;
#ifdef CONF_PMM_ENABLE
	SleepTimerInit() ;
#endif
#if (ENABLE_PDS == 1)
	PDS_Init() ;
#endif

	delay_ms(5) ;
	print_reset_causes() ;
#if (_DEBUG_ == 1)
	SYSTEM_AssertSubscribe(assertHandler);
#endif
	Stack_Init() ;
}

/*** configure_extint ***********************************************************
 \brief	Configures the ExternalInterrupt Controller to detect button state change
********************************************************************************/
static void configure_extint(void)
{
	struct extint_chan_conf eint_chan_conf ;
	extint_chan_get_config_defaults(&eint_chan_conf) ;	
	// button 0
	eint_chan_conf.gpio_pin           = BUTTON_0_EIC_PIN ;
	eint_chan_conf.gpio_pin_mux       = BUTTON_0_EIC_MUX ;
	eint_chan_conf.detection_criteria = EXTINT_DETECT_FALLING ;
	eint_chan_conf.filter_input_signal = true ;
	extint_chan_set_config(BUTTON_0_EIC_LINE, &eint_chan_conf) ;
}

/*** configure_eic_callback *****************************************************
 \brief	Configures and registers the Ext.Int callback function with the driver
********************************************************************************/
static void configure_eic_callback(void)
{
	// button 0
	extint_register_callback(
		extint_callback,
		BUTTON_0_EIC_LINE,
		EXTINT_CALLBACK_TYPE_DETECT
	) ;
	extint_chan_enable_callback(BUTTON_0_EIC_LINE, EXTINT_CALLBACK_TYPE_DETECT) ;
}

/*** configure_eic_callback *****************************************************
 \brief	Callback function for the EXTINT driver, called when an EXTINT occurs
********************************************************************************/
static void extint_callback(void)
{
	// Read the button level
	if (port_pin_get_input_level(BUTTON_0_PIN) == BUTTON_0_ACTIVE)
	{
#ifdef CONF_PMM_ENABLE
		PMM_Wakeup() ;
#endif
		// Wait for button debounce time
		delay_ms(APP_DEBOUNCE_TIME);
		// Check whether button is in default state
		while(port_pin_get_input_level(BUTTON_0_PIN) == BUTTON_0_ACTIVE)
		{
			delay_ms(500) ;
		}
	
		// Post task to application handler on button press
		buttonPressed = true ;
		SYSTEM_PostTask(APP_TASK_ID);
	}
}

/*********************************************************************//*
 \brief      Reads the DEV EUI if it is flashed in EDBG MCU
 ************************************************************************/
void dev_eui_read(void)
{
#ifndef CRYPTO_DEV_ENABLED
#if (EDBG_EUI_READ == 1)
	uint8_t invalidEDBGDevEui[8];
	uint8_t EDBGDevEUI[8];
	edbg_eui_read_eui64((uint8_t *)&EDBGDevEUI);
	memset(&invalidEDBGDevEui, 0xFF, sizeof(invalidEDBGDevEui));
	/* If EDBG does not have DEV EUI, the read value will be of all 0xFF, 
	   Set devEUI in conf_app.h in that case */
	if(0 != memcmp(&EDBGDevEUI, &invalidEDBGDevEui, sizeof(demoDevEui)))
	{
		/* Set EUI addr in EDBG if there */
		memcpy(demoDevEui, EDBGDevEUI, sizeof(demoDevEui));
	}
#endif
#endif
}

/*** appWakeup ******************************************************************
 \brief      Application Wake Up
 \param[in]  sleptDuration - slept duration in ms
********************************************************************************/
#ifdef CONF_PMM_ENABLE
static void appWakeup(uint32_t sleptDuration)
{
	HAL_Radio_resources_init();
	sio2host_init();
	printf("\r\nsleep_ok %ld ms\r\n", sleptDuration);
}
#endif

/*** app_resources_uninit *******************************************************
 \brief      Uninit. the application resources
********************************************************************************/
#ifdef CONF_PMM_ENABLE
static void app_resources_uninit(void)
{
	/* Disable USART TX and RX Pins */
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	pin_conf.powersave  = true;
	port_pin_set_config(HOST_SERCOM_PAD0_PIN, &pin_conf);
	port_pin_set_config(HOST_SERCOM_PAD1_PIN, &pin_conf);
	/* Disable UART module */
	sio2host_deinit();
	/* Disable Transceiver SPI Module */
	HAL_RadioDeInit();
}
#endif

/*** print_array ****************************************************************
 \brief      Function to Print array of characters
 \param[in]  *array  - Pointer of the array to be printed
 \param[in]   length - Length of the array
********************************************************************************/
static void print_array (uint8_t *array, uint8_t length)
{
    //printf("0x") ;
	for (uint8_t i =0; i < length; i++)
	{
        printf("%02x", *array) ;
        array++ ;
    }
    printf("\n\r") ;
}

/*** APP_TaskHandler ************************************************************
 \brief      Application task handler
********************************************************************************/
SYSTEM_TaskStatus_t APP_TaskHandler(void)
{
	if (buttonPressed)
	{
		buttonPressed = false ;
		buttonCount++ ;
		if (buttonCount > 65535) buttonCount = 0 ;
		printf("Button pressed %d times\r\n", buttonCount) ;
		send_data() ;
	}
	return SYSTEM_TASK_SUCCESS ;
}

/*** send_uplink ****************************************************************
 \brief      Function to transmit an uplink message
 \param[in]  type	- transmission type (LORAWAN_UNCNF or LORAWAN_CNF)
 \param[in]  fport	- 1-255
 \param[in]  data	- buffer to transmit
 \param[in]  len	- buffer length
 send_uplink(LORAWAN_UNCNF, 2, &temp_sen_str, strlen(temp_sen_str) - 1) ;
********************************************************************************/
static void send_uplink(TransmissionType_t type, uint8_t fport, void* data, uint8_t len)
{
	StackRetStatus_t status ;

	lorawanSendReq.buffer = data ;
	lorawanSendReq.bufferLength = len ;
	lorawanSendReq.confirmed = type ;	// LORAWAN_UNCNF or LORAWAN_CNF
	lorawanSendReq.port = fport ;		// fport [1-255]
	status = LORAWAN_Send(&lorawanSendReq) ;
	if (LORAWAN_SUCCESS == status)
	{
		printf("\r\nTrying to send uplink message\r\n") ;
		set_LED_data(LED_GREEN, &on) ;
		set_LED_data(LED_AMBER, &off) ;
	}
	else
	{
		set_LED_data(LED_GREEN, &off) ;
		set_LED_data(LED_AMBER, &on) ;
	}
}

/*** joindata_callback **********************************************************
 \brief      Callback function for the ending of Activation procedure
 \param[in]  status - join status
********************************************************************************/
void joindata_callback(StackRetStatus_t status)
{
	
#ifdef CRYPTO_DEV_ENABLED
	printf("DevEUI: ") ;
	LORAWAN_GetAttr(DEV_EUI, NULL, &demoDevEui) ;
	print_array((uint8_t *)&demoDevEui, sizeof(demoDevEui)) ;
#endif	
	
	StackRetStatus_t stackRetStatus = LORAWAN_INVALID_REQUEST ;
	
	// This is called every time the join process is finished
	if(LORAWAN_SUCCESS == status)
	{
		joined = true ;
		set_LED_data(LED_GREEN, &off) ;
		set_LED_data(LED_AMBER, &off) ;
		printf("\r\nJoin Successful!\r\n") ;
		
		printf("Press SW0 button to transmit an uplink message\r\n") ;
		
	}
	else if(LORAWAN_NO_CHANNELS_FOUND == status)
	{
		joined = false ;
		set_LED_data(LED_GREEN, &off) ;
		set_LED_data(LED_AMBER, &on) ;
		printf("\n No Free Channel found") ;
	}
	else if (LORAWAN_MIC_ERROR == status)
	{
		joined = false ;
		set_LED_data(LED_GREEN, &off) ;
		set_LED_data(LED_AMBER, &on) ;
		printf("\n MIC Error") ;
	}
	else if (LORAWAN_TX_TIMEOUT == status)
	{
		joined = false ;
		set_LED_data(LED_GREEN, &off) ;
		set_LED_data(LED_AMBER, &on) ;
		printf("\n Transmission Timeout") ;
	}
	else
	{
		joined = false;
		set_LED_data(LED_GREEN, &off) ;
		set_LED_data(LED_AMBER, &on) ;
		printf("\nJoining Denied\n\r") ;
	}

	if (joined == false)
	{
		printf("\nTry to join again ...\r\n") ;
		stackRetStatus = LORAWAN_Join(LORAWAN_OTAA) ;
		if (LORAWAN_SUCCESS == stackRetStatus)
		{
			set_LED_data(LED_GREEN, &on) ;
			printf("\nJoin Request sent to the network server\r\n") ;
		}
	}
}

/*** appdata_callback ***********************************************************
 \brief      Callback function for the ending of Bidirectional communication of
			 Application data
 \param[in]  *appHandle - callback handle
 \param[in]  *appData - callback parameters
********************************************************************************/
void appdata_callback(void *appHandle, appCbParams_t *appdata)
{
	StackRetStatus_t status = LORAWAN_INVALID_REQUEST;

	if (LORAWAN_EVT_RX_DATA_AVAILABLE == appdata->evt)
	{
		status = appdata->param.rxData.status;
		switch(status)
		{
			case LORAWAN_SUCCESS:
			{
				uint8_t *pData = appdata->param.rxData.pData ;
				uint8_t dataLength = appdata->param.rxData.dataLength ;
				if((dataLength > 0U) && (NULL != pData))
				{
					printf("*** Received DL Data ***\n\r") ;
					printf("\nFrame Received at port %d\n\r", pData[0]) ;
					printf("\nFrame Length - %d\n\r", dataLength) ;
					printf ("\nPayload: ") ;
					for (uint8_t i = 0; i < dataLength - 1; i++)
					{
						printf("%0x", pData[i+1]) ;
					}
					printf("\r\n*************************\r\n") ;		
				}
				else
				{
					printf("Received ACK for Confirmed data\r\n") ;
				}
			}
			break;
			case LORAWAN_RADIO_NO_DATA:
			{
				printf("\n\rRADIO_NO_DATA \n\r");
			}
			break;
			case LORAWAN_RADIO_DATA_SIZE:
			printf("\n\rRADIO_DATA_SIZE \n\r");
			break;
			case LORAWAN_RADIO_INVALID_REQ:
			printf("\n\rRADIO_INVALID_REQ \n\r");
			break;
			case LORAWAN_RADIO_BUSY:
			printf("\n\rRADIO_BUSY \n\r");
			break;
			case LORAWAN_RADIO_OUT_OF_RANGE:
			printf("\n\rRADIO_OUT_OF_RANGE \n\r");
			break;
			case LORAWAN_RADIO_UNSUPPORTED_ATTR:
			printf("\n\rRADIO_UNSUPPORTED_ATTR \n\r");
			break;
			case LORAWAN_RADIO_CHANNEL_BUSY:
			printf("\n\rRADIO_CHANNEL_BUSY \n\r");
			break;
			case LORAWAN_NWK_NOT_JOINED:
			printf("\n\rNWK_NOT_JOINED \n\r");
			break;
			case LORAWAN_INVALID_PARAMETER:
			printf("\n\rINVALID_PARAMETER \n\r");
			break;
			case LORAWAN_KEYS_NOT_INITIALIZED:
			printf("\n\rKEYS_NOT_INITIALIZED \n\r");
			break;
			case LORAWAN_SILENT_IMMEDIATELY_ACTIVE:
			printf("\n\rSILENT_IMMEDIATELY_ACTIVE\n\r");
			break;
			case LORAWAN_FCNTR_ERROR_REJOIN_NEEDED:
			printf("\n\rFCNTR_ERROR_REJOIN_NEEDED \n\r");
			break;
			case LORAWAN_INVALID_BUFFER_LENGTH:
			printf("\n\rINVALID_BUFFER_LENGTH \n\r");
			break;
			case LORAWAN_MAC_PAUSED :
			printf("\n\rMAC_PAUSED  \n\r");
			break;
			case LORAWAN_NO_CHANNELS_FOUND:
			printf("\n\rNO_CHANNELS_FOUND \n\r");
			break;
			case LORAWAN_BUSY:
			printf("\n\rBUSY\n\r");
			break;
			case LORAWAN_NO_ACK:
			printf("\n\rNO_ACK \n\r");
			break;
			case LORAWAN_NWK_JOIN_IN_PROGRESS:
			printf("\n\rALREADY JOINING IS IN PROGRESS \n\r");
			break;
			case LORAWAN_RESOURCE_UNAVAILABLE:
			printf("\n\rRESOURCE_UNAVAILABLE \n\r");
			break;
			case LORAWAN_INVALID_REQUEST:
			printf("\n\rINVALID_REQUEST \n\r");
			break;
			case LORAWAN_FCNTR_ERROR:
			printf("\n\rFCNTR_ERROR \n\r");
			break;
			case LORAWAN_MIC_ERROR:
			printf("\n\rMIC_ERROR \n\r");
			break;
			case LORAWAN_INVALID_MTYPE:
			printf("\n\rINVALID_MTYPE \n\r");
			break;
			case LORAWAN_MCAST_HDR_INVALID:
			printf("\n\rMCAST_HDR_INVALID \n\r");
			break;
			case LORAWAN_INVALID_PACKET:
			printf("\n\rINVALID_PACKET \n\r");
			break;
			default:
			printf("UNKNOWN ERROR\n\r");
			break;
		}
	}
	else if(LORAWAN_EVT_TRANSACTION_COMPLETE == appdata->evt)
	{
		switch(status = appdata->param.transCmpl.status)
		{
			case LORAWAN_SUCCESS:
			{
				printf("Transmission Success\r\n") ;
				set_LED_data(LED_GREEN, &off) ;
				set_LED_data(LED_AMBER, &off) ;
			}
			break;
			case LORAWAN_RADIO_SUCCESS:
			{
				printf("Transmission Success\r\n");
			}
			break;
			case LORAWAN_RADIO_NO_DATA:
			{
				printf("\n\rRADIO_NO_DATA \n\r");
			}
			break;
			case LORAWAN_RADIO_DATA_SIZE:
			printf("\n\rRADIO_DATA_SIZE \n\r");
			break;
			case LORAWAN_RADIO_INVALID_REQ:
			printf("\n\rRADIO_INVALID_REQ \n\r");
			break;
			case LORAWAN_RADIO_BUSY:
			printf("\n\rRADIO_BUSY \n\r");
			break;
			case LORAWAN_TX_TIMEOUT:
			printf("\nTx Timeout\n\r");
			break;
			case LORAWAN_RADIO_OUT_OF_RANGE:
			printf("\n\rRADIO_OUT_OF_RANGE \n\r");
			break;
			case LORAWAN_RADIO_UNSUPPORTED_ATTR:
			printf("\n\rRADIO_UNSUPPORTED_ATTR \n\r");
			break;
			case LORAWAN_RADIO_CHANNEL_BUSY:
			printf("\n\rRADIO_CHANNEL_BUSY \n\r");
			break;
			case LORAWAN_NWK_NOT_JOINED:
			printf("\n\rNWK_NOT_JOINED \n\r");
			break;
			case LORAWAN_INVALID_PARAMETER:
			printf("\n\rINVALID_PARAMETER \n\r");
			break;
			case LORAWAN_KEYS_NOT_INITIALIZED:
			printf("\n\rKEYS_NOT_INITIALIZED \n\r");
			break;
			case LORAWAN_SILENT_IMMEDIATELY_ACTIVE:
			printf("\n\rSILENT_IMMEDIATELY_ACTIVE\n\r");
			break;
			case LORAWAN_FCNTR_ERROR_REJOIN_NEEDED:
			printf("\n\rFCNTR_ERROR_REJOIN_NEEDED \n\r");
			break;
			case LORAWAN_INVALID_BUFFER_LENGTH:
			printf("\n\rINVALID_BUFFER_LENGTH \n\r");
			break;
			case LORAWAN_MAC_PAUSED :
			printf("\n\rMAC_PAUSED  \n\r");
			break;
			case LORAWAN_NO_CHANNELS_FOUND:
			printf("\n\rNO_CHANNELS_FOUND \n\r");
			break;
			case LORAWAN_BUSY:
			printf("\n\rBUSY\n\r");
			break;
			case LORAWAN_NO_ACK:
			printf("\n\rNO_ACK \n\r");
			break;
			case LORAWAN_NWK_JOIN_IN_PROGRESS:
			printf("\n\rALREADY JOINING IS IN PROGRESS \n\r");
			break;
			case LORAWAN_RESOURCE_UNAVAILABLE:
			printf("\n\rRESOURCE_UNAVAILABLE \n\r");
			break;
			case LORAWAN_INVALID_REQUEST:
			printf("\n\rINVALID_REQUEST \n\r");
			break;
			case LORAWAN_FCNTR_ERROR:
			printf("\n\rFCNTR_ERROR \n\r");
			break;
			case LORAWAN_MIC_ERROR:
			printf("\n\rMIC_ERROR \n\r");
			break;
			case LORAWAN_INVALID_MTYPE:
			printf("\n\rINVALID_MTYPE \n\r");
			break;
			case LORAWAN_MCAST_HDR_INVALID:
			printf("\n\rMCAST_HDR_INVALID \n\r");
			break;
			case LORAWAN_INVALID_PACKET:
			printf("\n\rINVALID_PACKET \n\r");
			break;
			default:
			printf("\n\rUNKNOWN ERROR\n\r");
			break;

		}
		printf("\n\r*************************************************\n\r");
	}
}

/*** appTimer_callback **********************************************************
 \brief      Callback function for the Application Timer called every APP_TIMEOUT
********************************************************************************/
void appTimer_callback(void)
{
	StackRetStatus_t status ;
	printf("App timer expired \r\n") ;
	if(false == joined)
	{
		// Not join - Send Join request
		status = LORAWAN_Join(LORAWAN_OTAA) ;
		if (LORAWAN_SUCCESS == status)
		{
			printf("Join Request Sent to the Network Server\n\r") ;
		}
	}
	else
	{
		send_data() ;
		SwTimerStart(AppTimerID, MS_TO_US(APP_TIMEOUT), SW_TIMEOUT_RELATIVE, (void*)appTimer_callback, NULL) ;
	}
}

/*********************************************************************//*
 \brief      Function to convert Celsius value to Fahrenheit
 \param[in]  cel_val   - Temperature value in Celsius
 \param[out] fauren_val- Temperature value in Fahrenheit
 ************************************************************************/
static float convert_celsius_to_fahrenheit(float celsius_val)
{
    float fauren_val;
    /* T(°F) = T(°C) × 9/5 + 32 */
    fauren_val = (((celsius_val * 9)/5) + 32);
    return fauren_val;
}

void send_data(void)
{
	float c_val ;	// variable contains T°celsius
	float f_val ;	// variable contains T°fahrenheit
	get_temp_sensor_data((uint8_t*)&c_val) ;
	f_val = convert_celsius_to_fahrenheit(c_val) ;
#ifndef CLONE
	sprintf(g_payload, "%s/%.1fC", first_name, c_val) ;
#else
	sprintf(g_payload, "$uperHacker/99.9C") ;
#endif
	printf("\r\nTemperature: %.1f\xf8 C/%.1f\xf8 F", c_val, f_val) ;
	printf("\r\nPayload    : %s\r\n", g_payload) ;
	
	send_uplink(LORAWAN_UNCNF, APP_PORT, g_payload, sizeof(g_payload)) ;		
}

#ifdef CRYPTO_DEV_ENABLED
static void print_ecc_info(void)
{
	ATCA_STATUS  status ;
	uint8_t    sn[9] ;			// ECC608A serial number (9 Bytes)
	uint8_t    info[2] ;
	//uint8_t    tkm_info[10] ;
	int      slot = 10 ;
	int      offset = 70 ;
	uint8_t appEUI[8] ;
	uint8_t devEUIascii[16] ;
	uint8_t devEUIdecoded[8] ;	// hex.
	size_t bin_size = sizeof(devEUIdecoded) ;

	// read the serial number
	status = atcab_read_serial_number(sn) ;
	printf("\r\n--------------------------------\r\n") ;

	// read the SE_INFO
	status = atcab_read_bytes_zone(ATCA_ZONE_DATA, slot, offset, info, sizeof(info)) ;
	
	// Read the CustomDevEUI
	status = atcab_read_bytes_zone(ATCA_ZONE_DATA, DEV_EUI_SLOT, 0, devEUIascii, 16) ;
	atcab_hex2bin((char*)devEUIascii, strlen((char*)devEUIascii), devEUIdecoded, &bin_size) ;

	printf("ECC608A Secure Element:\r\n") ;

	// Print DevEUI
	printf("DEV EUI       ") ;
	#if (SERIAL_NUM_AS_DEV_EUI == 1)
	print_array(sn, sizeof(sn)-1) ;
	#else
	print_array(devEUIdecoded, sizeof(devEUIdecoded)) ;
	#endif
	
	// Read the AppEUI
	status = atcab_read_bytes_zone(ATCA_ZONE_DATA, APP_EUI_SLOT, 0, appEUI, 8) ;
	printf("JOIN EUI      ") ;
	print_array(appEUI, sizeof(appEUI)) ;
	
	// assemble full TKM_INFO
/*	memcpy(tkm_info, info, 2) ;
	memcpy(&tkm_info[2], sn, 8) ;
	// tkm_info[] now contains the assembled tkm_info data
	printf("TKM INFO: ") ;
	print_array(tkm_info, sizeof(tkm_info)) ;
*/
	printf("SERIAL NUMBER ") ;
	print_array(sn, sizeof(sn)) ;
	
	
	printf("--------------------------------\r\n") ;
}
#endif

#include <ctype.h> // requires for isxdigit()

/*
 * \brief Converts the input string consisting of hexadecimal digits into an integer value
 */ 
int xtoi(char *c)
{
  size_t szlen = strlen(c);
  int idx, ptr, factor,result =0;

  if(szlen > 0){
    if(szlen > 8) return 0;
    result = 0;
    factor = 1;

    for(idx = szlen-1; idx >= 0; --idx){
    if(isxdigit( *(c+idx))){
	if( *(c + idx) >= 97){
	  ptr = ( *(c + idx) - 97) + 10;
	}else if( *(c + idx) >= 65){
	  ptr = ( *(c + idx) - 65) + 10;
	}else{
	  ptr = *(c + idx) - 48;
	}
	result += (ptr * factor);
	factor *= 16;
    }else{
		return 4;
    }
    }
  }

  return result;
}

uint8_t Parser_HexAsciiToInt(uint16_t hexAsciiLen, char* pInHexAscii, uint8_t* pOutInt)
{
	uint16_t rxHexAsciiLen = strlen(pInHexAscii);
	uint16_t iCtr = 0;
	uint16_t jCtr = rxHexAsciiLen >> 1;
	uint8_t retValue = 0;
	char tempBuff[3];
	if(rxHexAsciiLen % 2 == 0)
	{
		jCtr --;
	}
	if(hexAsciiLen == rxHexAsciiLen)
	{
		while(rxHexAsciiLen > 0)
		{
			if(rxHexAsciiLen >= 2U)
			{
				tempBuff[iCtr] = *(((char*)pInHexAscii) + (rxHexAsciiLen - 2));
				iCtr ++;
				tempBuff[iCtr] = *(((char*)pInHexAscii) + (rxHexAsciiLen - 1));

				rxHexAsciiLen -= 2U;
			}
			else
			{
				tempBuff[iCtr] = '0';
				iCtr ++;
				tempBuff[iCtr] = *(((char*)pInHexAscii) + (rxHexAsciiLen - 1));

				rxHexAsciiLen --;
			}

			iCtr ++;
			tempBuff[iCtr] = '\0';
			*(pOutInt + jCtr) = xtoi(tempBuff);
			iCtr = 0;
			jCtr --;
		}

		retValue = 1;
	}
	return retValue;
}


static void provisioning(void)
{
	bool exit = false ;
	int rxChar ;
	char serialData ;
	uint8_t cntChar ;
	uint8_t maxChar ;
	uint8_t step = 0 ;
	char buffer[50] ;

	while (!exit)
	{
		if (step == 0)
		{
			maxChar = 16 ;
			cntChar = 0 ;
			memset(buffer, 0, sizeof(buffer)) ;
			printf("Start provisioning!\r\n") ;
			printf("Enter DevEui [hex 8-bytes/16-char]: ") ;
			step++ ;
		}
		/* verify if there was any character received */
		if ((-1) != (rxChar = sio2host_getchar_nowait()))
		{
			serialData = (char)rxChar ;

			if (((serialData >= '0') && (serialData <= '9')) || ((serialData >= 'a') && (serialData <= 'f')) || ((serialData >= 'A') && (serialData <= 'F')))
			{
				buffer[cntChar] = serialData ;
				cntChar++ ;
			}
			if (cntChar == maxChar)
			{
				cntChar = 0 ;
				printf("\r\n") ;
				//printf("\r\nPress Enter to continue ... ") ;
				//while (sio2host_getchar_nowait() != '\r') continue ;
				if (step == 1 )
				{
					// store devEui
					Parser_HexAsciiToInt(16, buffer, (uint8_t *)demoDevEui) ;
					printf("Enter JoinEui [hex 8-bytes/16-char]: ") ;
					step++ ;
				}
				else if (step == 2)
				{
					// store appEui
					Parser_HexAsciiToInt(16, buffer, (uint8_t *)demoAppEui) ;
					printf("Enter AppKey [hex 16-bytes/32-char]: ") ;
					maxChar = 32 ;
					step++ ;
				}
				else if (step == 3)
				{
					// store appKey
					Parser_HexAsciiToInt(32U, buffer, (uint8_t *)demoAppKey) ;
					bool out = false ;
					step = 0 ;
					while(!out)
					{
						if (step == 0)
						{
							printf("\r\n") ;
							printf("DevEui: ") ;
							print_array(demoDevEui, sizeof(demoDevEui)) ;
							printf("JoinEui: ") ;
							print_array(demoAppEui, sizeof(demoAppEui)) ;
							printf("AppKey: ") ;
							print_array(demoAppKey, sizeof(demoAppKey)) ;
							printf("\r\n1. Confirm the provisioning\r\n") ;
							printf("2. Modify the provisioning\r\n") ;
							step++ ;
							
							rxChar = sio2host_getchar_nowait() ;	// previous \r
							rxChar = sio2host_getchar_nowait() ;	// previous \n
							
						}
						if ((-1) != (rxChar = sio2host_getchar_nowait()))
						{
							serialData = (char)rxChar ;
							if (serialData == '1')
							{
								printf("Provisioning done!\r\n") ;
								out = true ;
								exit = true ;
							}
							else if (serialData == '2')
							{
								out = true ;
								step = 0 ;
							}
							else
							{
								step = 0 ;
							}
						}
					}
				}
			}
		}
	}
}

static void enter_first_name(void)
{
	bool exit = false ;
	int rxChar ;
	char serialData ;
	uint8_t cntChar ;
	uint8_t maxChar ;
	uint8_t step = 0 ;

	while (!exit)
	{
		if (step == 0)
		{
	rxChar = sio2host_getchar_nowait() ;	// previous \r
	rxChar = sio2host_getchar_nowait() ;	// previous \n

			
			maxChar = 10 ;
			cntChar = 0 ;
			memset(first_name, 0, sizeof(first_name)) ;
			printf("\r\n") ;
			printf("Enter your first name [10char max.] and press enter: ") ;
			step++ ;
		}
		/* verify if there was any character received */
		if ((-1) != (rxChar = sio2host_getchar_nowait()))
		{
			serialData = (char)rxChar ;
			if (((serialData >= 'a') && (serialData <= 'z')) || ((serialData >= 'A') && (serialData <= 'Z')))
			{
				first_name[cntChar] = serialData ;
				cntChar++ ;				
				if (cntChar == maxChar)
				{
					first_name_len = cntChar ;
					printf("\r\n") ;
					exit = true ;
				}
			}
			if (serialData == 0x0D || serialData == 0x0A || serialData == '\0')
			{
				// enter key
				first_name_len = cntChar ;
				printf("\r\n") ;
				exit = true ;
			}
		}
	}	
}

static void select_lab(void)
{
	bool exit = false ;
	int rxChar ;
	char serialData ;
	uint8_t step = 0 ;

	while (!exit)
	{
		if (step == 0)
		{
			printf("\r\n") ;
			printf("1. Lab1\r\n") ;
			printf("2. Lab2\r\n") ;
			printf("Select which lab you want to start: ") ;
			step++ ;
		}
		/* verify if there was any character received */
		if ((-1) != (rxChar = sio2host_getchar_nowait()))
		{
			serialData = (char)rxChar ;
			if (serialData == 0x31)
			{
				currentApp = LAB1_APP ;
				exit = true ;
			}
			else if (serialData == 0x32)
			{
				currentApp = LAB2_APP ;
				exit = true ;
			}
			else
			{
				printf(" >> Incorrect input\r\n") ;
				step = 0 ;
			}
		}
	}
}

static void press_enter(void)
{
	bool exit = false ;
	int rxChar ;
	char serialData ;
	uint8_t step = 0 ;

	while (!exit)
	{
		if (step == 0)
		{
			printf("Press enter to continue ...") ;
			step++ ;
		}
		/* verify if there was any character received */
		if ((-1) != (rxChar = sio2host_getchar_nowait()))
		{
			serialData = (char)rxChar ;
			if (serialData == 0x13)
			{
				exit = true ;
			}
		}
	}
}


/*** main ***********************************************************************
 \brief      Main function
********************************************************************************/
int main(void)
{
	bool useECC608 = false ;
	
	StackRetStatus_t status = LORAWAN_INVALID_REQUEST ;
	
	// --------------------------------------------------------------------------
	// Init. section
	// --------------------------------------------------------------------------	
	app_init() ;
	resource_init() ;

	set_LED_data(LED_GREEN, &off) ;
	set_LED_data(LED_AMBER, &off) ;

	printf("\r\n-- ATSAMR34 LoRaWAN Application --\r\n") ;
	printf("The Things Conference 2020\r\n") ;

	select_lab() ;
	
	if (currentApp == LAB1_APP) useECC608 = false ;
	else useECC608 = true ;
		
	/* Initializes the Security modules */
	if (SAL_SUCCESS != SAL_Init(useECC608))
	{
		printf("Initialization of Security module is failed\r\n");
		/* Stop Further execution */
		while (1) {
		}
	}

	if (!useECC608)
	{
		// Lab 1
		printf("\r\nStart Lab1...\r\n") ;
#ifndef CLONE
		provisioning() ;
#endif
	}
	else
	{
		// Lab 2
		printf("\r\nStart Lab2...\r\n") ;
#ifdef CRYPTO_DEV_ENABLED
		print_ecc_info() ;			
#endif
	}

#ifndef CLONE
	enter_first_name() ;
#endif

	LORAWAN_Init(appdata_callback, joindata_callback) ;

	// Select your band here
	LORAWAN_Reset(ISM_EU868) ;

	// Select the class of your device
	EdClass_t classType = CLASS_A ;
	status = LORAWAN_SetAttr(EDCLASS, &classType) ;

	// Select the data rate
	uint8_t datarate = DR5 ;
	status = LORAWAN_SetAttr(CURRENT_DATARATE, &datarate) ;

	if (useECC608)
	{
#ifdef CRYPTO_DEV_ENABLED
		bool cryptoDevEnabled = true ;
		LORAWAN_SetAttr(CRYPTODEVICE_ENABLED, &cryptoDevEnabled) ;
#endif
	}
	else
	{
		status = LORAWAN_SetAttr(DEV_EUI, demoDevEui) ;
		status = LORAWAN_SetAttr(JOIN_EUI, demoAppEui) ;
		status = LORAWAN_SetAttr(APP_KEY, demoAppKey) ;
	}

	// Set ADR or not
	bool adr = false ;
	status = LORAWAN_SetAttr(ADR, &adr) ;

	// Join the network
	status = LORAWAN_Join(LORAWAN_OTAA) ;
	if (LORAWAN_SUCCESS == status)
	{
		set_LED_data(LED_GREEN, &on) ;
		printf("Join Request sent to the network server\r\n") ;
	}

    while (1)
    {
        SYSTEM_RunTasks();
#ifdef CONF_PMM_ENABLE
		PMM_SleepReq_t sleepReq ;
		/* Put the application to sleep */
		sleepReq.sleepTimeMs = DEMO_CONF_DEFAULT_APP_SLEEP_TIME_MS ;
		sleepReq.pmmWakeupCallback = appWakeup ;
		sleepReq.sleep_mode = CONF_PMM_SLEEPMODE_WHEN_IDLE ;
		if (CONF_PMM_SLEEPMODE_WHEN_IDLE == SLEEP_MODE_STANDBY)
		{
			deviceResetsForWakeup = false ;
		}
		if (true == LORAWAN_ReadyToSleep(deviceResetsForWakeup))
		{
			app_resources_uninit();
			if (PMM_SLEEP_REQ_DENIED == PMM_Sleep(&sleepReq))
			{
				HAL_Radio_resources_init();
				sio2host_init();
				/*printf("\r\nsleep_not_ok\r\n");*/
			}
		}
#endif
	}
	return 0 ;
}

/**
 End of File
 */
