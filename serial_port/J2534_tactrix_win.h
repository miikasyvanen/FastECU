//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2004- Tactrix Inc.
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// "OP2.0:" refers to items that may have special relevance to the Tactrix OpenPort 2.0

#ifdef __cplusplus
   #define EXTERN_C     extern "C"
#else
   #define EXTERN_C     extern
#endif

/////////////////
// API Functions
/////////////////

#ifdef OP20PT32_LIB
#define PT_CALL 
#define PT_API EXTERN_C
#else
#if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
#define PT_CALL __stdcall
#else
#define PT_CALL
#endif
#define PT_API EXTERN_C
#endif

typedef long (PT_CALL PF_PassThruOpen)(const void *, unsigned long *);
typedef long (PT_CALL PF_PassThruClose)(unsigned long);
typedef long (PT_CALL PF_PassThruConnect)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long *);
typedef long (PT_CALL PF_PassThruDisconnect)(unsigned long);
typedef long (PT_CALL PF_PassThruReadMsgs)(unsigned long, void *, unsigned long *, unsigned long);
typedef long (PT_CALL PF_PassThruWriteMsgs)(unsigned long, const void *, unsigned long *, unsigned long);
typedef long (PT_CALL PF_PassThruStartPeriodicMsg)(unsigned long, const void *, unsigned long *, unsigned long);
typedef long (PT_CALL PF_PassThruStopPeriodicMsg)(unsigned long, unsigned long);
typedef long (PT_CALL PF_PassThruStartMsgFilter)(unsigned long, unsigned long, const void *, const void *, const void *, unsigned long *);
typedef long (PT_CALL PF_PassThruStopMsgFilter)(unsigned long, unsigned long);
typedef long (PT_CALL PF_PassThruSetProgrammingVoltage)(unsigned long, unsigned long, unsigned long);
typedef long (PT_CALL PF_PassThruReadVersion)(unsigned long, char *, char *, char *);
typedef long (PT_CALL PF_PassThruGetLastError)(char *);
typedef long (PT_CALL PF_PassThruIoctl)(unsigned long, unsigned long, const void *, void *);

typedef void (*PF_StatusCallback)(const char *,int,int);


////////////////
// Protocol IDs
////////////////

// J2534-1
#define J1850VPW						1
#define J1850PWM						2
#define ISO9141							3
#define ISO14230						4
#define CAN								5
#define ISO15765						6
#define SCI_A_ENGINE					7	// OP2.0: Not supported
#define SCI_A_TRANS						8	// OP2.0: Not supported
#define SCI_B_ENGINE					9	// OP2.0: Not supported
#define SCI_B_TRANS						10	// OP2.0: Not supported

// J2534-2
#define CAN_CH1							0x00009000
#define J1850VPW_CH1					0x00009080
#define J1850PWM_CH1					0x00009160
#define ISO9141_CH1						0x00009240
#define ISO9141_CH2						0x00009241
#define ISO9141_CH3						0x00009242
#define ISO9141_K						ISO9141_CH1
#define ISO9141_L						ISO9141_CH2		// OP2.0: Support for ISO9141 communications over the L line
#define ISO9141_INNO					ISO9141_CH3		// OP2.0: Support for RS-232 receive-only communications via the 2.5mm jack
#define ISO14230_CH1					0x00009320
#define ISO14230_CH2					0x00009321
#define ISO14230_K						ISO14230_CH1
#define ISO14230_L						ISO14230_CH2	// OP2.0: Support for ISO14230 communications over the L line
#define ISO15765_CH1					0x00009400


/////////////
// IOCTL IDs
/////////////

// J2534-1
#define GET_CONFIG								0x01
#define SET_CONFIG								0x02
#define READ_VBATT								0x03
#define FIVE_BAUD_INIT							0x04
#define FAST_INIT								0x05
#define CLEAR_TX_BUFFER							0x07
#define CLEAR_RX_BUFFER							0x08
#define CLEAR_PERIODIC_MSGS						0x09
#define CLEAR_MSG_FILTERS						0x0A
#define CLEAR_FUNCT_MSG_LOOKUP_TABLE			0x0B	// OP2.0: Not yet supported
#define ADD_TO_FUNCT_MSG_LOOKUP_TABLE			0x0C 	// OP2.0: Not yet supported
#define DELETE_FROM_FUNCT_MSG_LOOKUP_TABLE		0x0D 	// OP2.0: Not yet supported
#define READ_PROG_VOLTAGE						0x0E	// OP2.0: Several pins are supported

// J2534-2
#define SW_CAN_NS								0x8000 // OP2.0: Not supported
#define SW_CAN_HS								0x8001 // OP2.0: Not supported

// Tactrix specific IOCTLs
#define TX_IOCTL_BASE							0x70000
// OP2.0: The IOCTL below supports application-specific functions
// that can be built into the hardware
#define TX_IOCTL_APP_SERVICE					(TX_IOCTL_BASE+0)
#define TX_IOCTL_SET_DLL_DEBUG_FLAGS			(TX_IOCTL_BASE+1)
#define TX_IOCTL_DLL_DEBUG_FLAG_J2534_CALLS		0x00000001
#define TX_IOCTL_DLL_DEBUG_FLAG_ALL_DEV_COMMS	0x00000002
#define TX_IOCTL_SET_DEV_DEBUG_FLAGS			(TX_IOCTL_BASE+2)
#define TX_IOCTL_DEV_DEBUG_FLAG_USB_COMMS		0x00000001
#define TX_IOCTL_SET_DLL_STATUS_CALLBACK		(TX_IOCTL_BASE+3)
#define TX_IOCTL_GET_DEVICE_INSTANCES    		(TX_IOCTL_BASE+4)

/////////////////
// Pin numbering
/////////////////

#define AUX_PIN			0 	// aux jack	OP2.0: Supports GND and adj. voltage
#define J1962_PIN_1		1	//			OP2.0: Supports GND and adj. voltage
#define J1962_PIN_2		2	// J1850P	OP2.0: Supports 5V and 8V
#define J1962_PIN_3		3	//			OP2.0: Supports GND and adj. voltage
#define J1962_PIN_4		4	// GND
#define J1962_PIN_5		5	// GND
#define J1962_PIN_6		6	// CAN
#define J1962_PIN_7		7	// K		OP2.0: Supports GND
#define J1962_PIN_8		8	//			OP2.0: Supports reading voltage
#define J1962_PIN_9		9	//			OP2.0: Supports GND and adj. voltage
#define J1962_PIN_10	10	// J1850M	OP2.0: Supports GND
#define J1962_PIN_11	11	//			OP2.0: Supports GND and adj. voltage
#define J1962_PIN_12	12	//			OP2.0: Supports GND and adj. voltage
#define J1962_PIN_13	13 	//			OP2.0: Supports GND and adj. voltage
#define J1962_PIN_14	14	// CAN
#define J1962_PIN_15	15	// L		OP2.0: Supports GND
#define J1962_PIN_16	16	// VBAT		OP2.0: Supports reading voltage
#define PIN_VADJ		17	// internal	OP2.0: Supports reading voltage

////////////////////////////////
// Special pin voltage settings
////////////////////////////////

#define SHORT_TO_GROUND						0xFFFFFFFE
#define VOLTAGE_OFF							0xFFFFFFFF


/////////////////////////////////////////
// GET_CONFIG / SET_CONFIG Parameter IDs
/////////////////////////////////////////

// J2534-1
#define DATA_RATE						0x01
#define LOOPBACK						0x03
#define NODE_ADDRESS					0x04 // OP2.0: Not yet supported
#define NETWORK_LINE					0x05 // OP2.0: Not yet supported
#define P1_MIN							0x06 // J2534 says this may not be changed
#define P1_MAX							0x07
#define P2_MIN							0x08 // J2534 says this may not be changed
#define P2_MAX							0x09 // J2534 says this may not be changed
#define P3_MIN							0x0A
#define P3_MAX							0x0B // J2534 says this may not be changed
#define P4_MIN							0x0C
#define P4_MAX							0x0D // J2534 says this may not be changed
#define W0								0x19
#define W1								0x0E
#define W2								0x0F
#define W3								0x10
#define W4								0x11
#define W5								0x12
#define TIDLE							0x13
#define TINIL							0x14
#define TWUP							0x15
#define PARITY							0x16
#define BIT_SAMPLE_POINT				0x17 // OP2.0: Not yet supported
#define SYNC_JUMP_WIDTH					0x18 // OP2.0: Not yet supported
#define T1_MAX							0x1A
#define T2_MAX							0x1B
#define T3_MAX							0x24
#define T4_MAX							0x1C
#define T5_MAX							0x1D
#define ISO15765_BS						0x1E
#define ISO15765_STMIN					0x1F
#define DATA_BITS						0x20
#define FIVE_BAUD_MOD					0x21
#define BS_TX							0x22
#define STMIN_TX						0x23
#define ISO15765_WFT_MAX				0x25

// J2534-2
#define CAN_MIXED_FORMAT				0x8000 
#define J1962_PINS						0x8001 // OP2.0: Not supported
#define SW_CAN_HS_DATA_RATE				0x8010 // OP2.0: Not supported
#define SW_CAN_SPEEDCHANGE_ENABLE		0x8011 // OP2.0: Not supported
#define SW_CAN_RES_SWITCH				0x8012 // OP2.0: Not supported
#define ACTIVE_CHANNELS					0x8020 // OP2.0: Not supported
#define SAMPLE_RATE						0x8021 // OP2.0: Not supported
#define SAMPLES_PER_READING				0x8022 // OP2.0: Not supported
#define READINGS_PER_MSG				0x8023 // OP2.0: Not supported
#define AVERAGING_METHOD				0x8024 // OP2.0: Not supported
#define SAMPLE_RESOLUTION				0x8025 // OP2.0: Not supported
#define INPUT_RANGE_LOW					0x8026 // OP2.0: Not supported
#define INPUT_RANGE_HIGH				0x8027 // OP2.0: Not supported

// Tactrix specific parameter IDs
#define TX_PARAM_BASE					0x9000
#define TX_PARAM_STOP_BITS				(TX_PARAM_BASE_BASE+0)


//////////////////////
// PARITY definitions
//////////////////////

#define NO_PARITY							0
#define ODD_PARITY							1
#define EVEN_PARITY							2

////////////////////////////////
// CAN_MIXED_FORMAT definitions
////////////////////////////////

#define CAN_MIXED_FORMAT_OFF						0
#define CAN_MIXED_FORMAT_ON							1
#define CAN_MIXED_FORMAT_ALL_FRAMES					2


/////////////
// Error IDs
/////////////

// J2534-1
#define ERR_SUCCESS							0x00
#define STATUS_NOERROR						0x00
#define ERR_NOT_SUPPORTED					0x01
#define ERR_INVALID_CHANNEL_ID				0x02
#define ERR_INVALID_PROTOCOL_ID				0x03
#define ERR_NULL_PARAMETER					0x04
#define ERR_INVALID_IOCTL_VALUE				0x05
#define ERR_INVALID_FLAGS					0x06
#define ERR_FAILED							0x07
#define ERR_DEVICE_NOT_CONNECTED			0x08
#define ERR_TIMEOUT							0x09
#define ERR_INVALID_MSG						0x0A
#define ERR_INVALID_TIME_INTERVAL			0x0B
#define ERR_EXCEEDED_LIMIT					0x0C
#define ERR_INVALID_MSG_ID					0x0D
#define ERR_DEVICE_IN_USE					0x0E
#define ERR_INVALID_IOCTL_ID				0x0F
#define ERR_BUFFER_EMPTY					0x10
#define ERR_BUFFER_FULL						0x11
#define ERR_BUFFER_OVERFLOW					0x12
#define ERR_PIN_INVALID						0x13
#define ERR_CHANNEL_IN_USE					0x14
#define ERR_MSG_PROTOCOL_ID					0x15
#define ERR_INVALID_FILTER_ID				0x16
#define ERR_NO_FLOW_CONTROL					0x17
#define ERR_NOT_UNIQUE						0x18
#define ERR_INVALID_BAUDRATE				0x19
#define ERR_INVALID_DEVICE_ID				0x1A

// OP2.0 Tactrix specific
#define ERR_OEM_VOLTAGE_TOO_LOW				0x78 // OP2.0: the requested output voltage is lower than the OP2.0 capabilities
#define ERR_OEM_VOLTAGE_TOO_HIGH			0x77 // OP2.0: the requested output voltage is higher than the OP2.0 capabilities


/////////////////////////
// PassThruConnect flags
/////////////////////////

#define CAN_29BIT_ID						0x00000100
#define ISO9141_NO_CHECKSUM					0x00000200
#define CAN_ID_BOTH							0x00000800
#define ISO9141_K_LINE_ONLY					0x00001000
#define SNIFF_MODE							0x10000000 // OP2.0: listens to a bus (e.g. CAN) without acknowledging

//////////////////
// RxStatus flags
//////////////////

#define TX_MSG_TYPE							0x00000001
#define START_OF_MESSAGE					0x00000002
#define ISO15765_FIRST_FRAME				0x00000002
#define RX_BREAK							0x00000004
#define TX_DONE								0x00000008
#define ISO15765_PADDING_ERROR				0x00000010
#define ISO15765_EXT_ADDR					0x00000080
#define ISO15765_ADDR_TYPE					0x00000080
//#define CAN_29BIT_ID						0x00000100 // (already defined above)

//////////////////
// TxStatus flags
//////////////////

#define ISO15765_FRAME_PAD					0x00000040
//#define ISO15765_ADDR_TYPE				0x00000080 // (already defined above)
//#define CAN_29BIT_ID						0x00000100 // (already defined above)
#define WAIT_P3_MIN_ONLY					0x00000200
#define SW_CAN_HV_TX						0x00000400 // OP2.0: Not supported
#define SCI_MODE							0x00400000 // OP2.0: Not supported
#define SCI_TX_VOLTAGE						0x00800000 // OP2.0: Not supported


////////////////
// Filter types
////////////////

#define PASS_FILTER							0x00000001
#define BLOCK_FILTER						0x00000002
#define FLOW_CONTROL_FILTER					0x00000003


/////////////////
// Message struct
/////////////////

#define PASSTHRU_MSG_DATA_SIZE 4128

typedef struct
{
	unsigned long ProtocolID;
	unsigned long RxStatus;
	unsigned long TxFlags;
	unsigned long Timestamp;
	unsigned long DataSize;
	unsigned long ExtraDataIndex;
    unsigned char Data[PASSTHRU_MSG_DATA_SIZE];
} PASSTHRU_MSG;


////////////////
// IOCTL structs
////////////////

typedef struct
{
	unsigned long Parameter;
	unsigned long Value;
} SCONFIG;

typedef struct
{
	unsigned long NumOfParams;
	SCONFIG *ConfigPtr;
} SCONFIG_LIST;

typedef struct
{
	unsigned long NumOfBytes;
	unsigned char *BytePtr;
} SBYTE_ARRAY;



