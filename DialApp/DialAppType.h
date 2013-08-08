/********************************************************************************************\
 Library     :  HFP DialApp (Bluetooth HFP library)
 Filename    :  DialAppType.h
 Purpose     :  Public h-file for DialApp.dll - types information
\********************************************************************************************/

#ifndef _DIALAPPTYPES_H
#define _DIALAPPTYPES_H


/********************************************************************************************\
						Common types (duplicated from def.h)
\********************************************************************************************/

typedef const char			cchar;
typedef const wchar_t		wchar;
typedef unsigned			uint32;
typedef unsigned long long	uint64;



/********************************************************************************************\
								Public data types
\********************************************************************************************/

/*
 *************************************************************************************
 DialApp error codes.
 May be thrown as integer C++ exception from all functions.
 *************************************************************************************
 */
enum DialAppError
{
	// Common errors
	DialAppError_Ok							,
	DialAppError_InternalError				,
	DialAppError_InsufficientResources		,

	// Init errors
	DialAppError_InitBluetoothRadioError	,
	DialAppError_InitDriverError			,

	// HFP API errors
	DialAppError_UnknownDevice				,
	DialAppError_IncorrectState4Call		,
	DialAppError_ConnectFailure				,
	DialAppError_ServiceConnectFailure		,
	DialAppError_CallFailure				,

	// Audio channel errors
	DialAppError_OpenScoFailure				,
	DialAppError_ReadWriteScoError			,
	DialAppError_WaveApiError				,
};



/*
 *************************************************************************************
 DialApp states, passed to a host application via DialAppCb each time the state 
 is changed. Also, dialappGetCurState func gets this enum values.
 *************************************************************************************
 */
enum DialAppState
{
	DialAppState_IdleNoDevice,
	DialAppState_DisconnectedDevicePresent,
	DialAppState_Connecting,
	DialAppState_Connected,
	DialAppState_ServiceConnecting,
	DialAppState_ServiceConnected,
	DialAppState_Calling,
	DialAppState_Ringing,
	DialAppState_InCallPcSoundOff,
	DialAppState_InCallPcSoundOn
};


/*
 *************************************************************************************
 Bluetooth device information type.
 *************************************************************************************
 */
struct DialAppBthDev
{
	uint64	 Address;
	wchar  	*Name;
};



/*
 *************************************************************************************
 DIALAPP_FLAG_... bits correspondent to DialAppParam fields and passed as one 32-bit 
 bitmask flag to DialAppCb. Each bit corresponds to one of the fields and means that 
 this field was changed before current callback call.
 The only DIALAPP_FLAG_INITSTATE and DIALAPP_FLAG_NEWSTATE bits is not related to param,
 but to the SM's states changes.
 *************************************************************************************
 */
#define DIALAPP_FLAG_PCSOUND			0x01	// DialAppParam.PcSound was changed
#define DIALAPP_FLAG_CURDEV				0x02	// DialAppParam.CurDevice was changed
#define DIALAPP_FLAG_ABONENT			0x04	// DialAppParam.AbonentNumber & AbonentName was set
#define DIALAPP_FLAG_NEWSTATE	  0x40000000	// Set when current state was changed
#define DIALAPP_FLAG_INITSTATE	  0x80000000	// Set one-time when the SM started, in 1st callback only, before entering to the idle state (when SM's data, e.g. paired device list, are already initialized)



/*
 *************************************************************************************
 Union which contains different parameters passed to DialAppCb. 
 The concrete parameter depends on DialAppState.
 *************************************************************************************
 */
struct DialAppParam
{
	bool			PcSound;		// PcSound = On/Off: all messages may contain this field (DIALAPP_FLAG_PCSOUND flag)
	DialAppBthDev	*CurDevice;		// Device address (DIALAPP_FLAG_CURDEV flag)
	char			*AbonentNumber;	// Calling/called abonent Number (DIALAPP_FLAG_ABONENT flag)
	char			*AbonentName;	// Calling/called abonent Name (may be 0, DIALAPP_FLAG_ABONENT flag)
};



/*
 *************************************************************************************
 DialApp asynchronous callback function prototype. The correspondent function should 
 be passed to dialappInit and will receive notifications about all device state changes 
 and asynchronous errors. This function is called each time when the internal State 
 Machine's state was changed. In the case this state change was caused by any 
 failure, the DialAppError status will be set to correspondent error code (not equal 
 to DialAppError_Ok).
 The flag parameter is a bitmask with bits set which correspond to param fields 
 changed just before this callback call (see DIALAPP_FLAG_... description).
 The only DIALAPP_FLAG_NEWSTATE flag's bit is not related to param and is set when 
 the current state was changed.
 *************************************************************************************
 */
typedef void (*DialAppCb) (DialAppState state, DialAppError status, uint32 flags, DialAppParam* param);



#endif // _DIALAPPTYPES_H
