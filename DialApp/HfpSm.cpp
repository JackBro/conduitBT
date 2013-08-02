/********************************************************************************\
 Filename    :  HfpSm.cpp
 Purpose     :  Main HFP State Machine
\********************************************************************************/

#include "def.h"
#include "deblog.h"
#include "HfpSm.h"
#include "smBody.h"


IMPL_STATES (HfpSm, STATE_LIST_HFPSM)

HfpSm		HfpSmObj;
HfpSmCb  	HfpSm::UserCallback;


void HfpSm::Construct()
{
	MyTimer.Construct ();
	SMT<HfpSm>::Construct (SM_HFP);
	ScoAppObj = new ScoApp();
	ScoAppObj->Construct();
}


void HfpSm::Destruct()
{
	delete ScoAppObj;
	MyTimer.Destruct();
}


void HfpSm::Init (DialAppCb cb)
{
    /*---------------------------------- STATE: Idle ---------------------------------------------------*/
    InitStateNode (STATE_Idle,				SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
    InitStateNode (STATE_Idle,				SMEV_ForgetDevice,				STATE_Idle,				NULLTRANS);
    InitStateNode (STATE_Idle,				SMEV_StartOutgoingCall,			STATE_Idle,				&IncorrectState4Call);
    /*-------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: Disconnected ------------------------------------------*/
    InitStateNode (STATE_Disconnected,		SMEV_Failure,					STATE_Idle,				NULLTRANS);
    InitStateNode (STATE_Disconnected,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
    InitStateNode (STATE_Disconnected,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
    InitStateNode (STATE_Disconnected,		SMEV_ConnectStart,				STATE_Connecting,		&Connect);
    InitStateNode (STATE_Disconnected,		SMEV_Timeout,					STATE_Connecting,		&Connect);
    InitStateNode (STATE_Disconnected,		SMEV_StartOutgoingCall,			STATE_Disconnected,		&IncorrectState4Call);
    /*-------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: Connecting   ------------------------------------------*/
    InitStateNode (STATE_Connecting,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
    InitStateNode (STATE_Connecting,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
    InitStateNode (STATE_Connecting,		SMEV_Disconnected,				STATE_Disconnected,		&Disconnected);
    InitStateNode (STATE_Connecting,		SMEV_Failure,					STATE_Disconnected,		&ConnectFailure);
    InitStateNode (STATE_Connecting,		SMEV_Connected,					STATE_Connected,		&Connected);
    InitStateNode (STATE_Connecting,		SMEV_StartOutgoingCall,			STATE_Connecting,		&IncorrectState4Call);
    /*-------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: Connected   -------------------------------------------*/
    InitStateNode (STATE_Connected,			SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
    InitStateNode (STATE_Connected,			SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
    InitStateNode (STATE_Connected,			SMEV_Disconnected,				STATE_Disconnected,		&Disconnected);
    InitStateNode (STATE_Connected,			SMEV_HfpConnectStart,			STATE_HfpConnecting,	&HfpConnect);
    InitStateNode (STATE_Connected,			SMEV_StartOutgoingCall,			STATE_Connected,		&IncorrectState4Call);
    /*-------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: HfpConnecting   ---------------------------------------*/
    InitStateNode (STATE_HfpConnecting,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
    InitStateNode (STATE_HfpConnecting,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
    InitStateNode (STATE_HfpConnecting,		SMEV_Disconnected,				STATE_Disconnected,		&Disconnected);
    InitStateNode (STATE_HfpConnecting,		SMEV_Failure,					STATE_Disconnected,		&ServiceConnectFailure);
    InitStateNode (STATE_HfpConnecting,		SMEV_HfpConnected,				STATE_HfpConnected,		&HfpConnected);
    InitStateNode (STATE_HfpConnecting,		SMEV_Timeout,					STATE_HfpConnected,		&HfpConnected);
    InitStateNode (STATE_HfpConnecting,		SMEV_StartOutgoingCall,			STATE_HfpConnecting,	&IncorrectState4Call);
    InitStateNode (STATE_HfpConnecting,		SMEV_AtResponse,				STATE_HfpConnecting,	&AtProcessing);
    /*------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: HfpConnected   ----------------------------------------*/
    InitStateNode (STATE_HfpConnected,		SMEV_ForgetDevice,				STATE_Idle,				&ForgetDevice);
    InitStateNode (STATE_HfpConnected,		SMEV_SelectDevice,				STATE_Disconnected,		&SelectDevice);
    InitStateNode (STATE_HfpConnected,		SMEV_Disconnected,				STATE_Disconnected,		&Disconnected);
    InitStateNode (STATE_HfpConnected,		SMEV_EndCall,					STATE_HfpConnected,		NULLTRANS);
	InitStateNode (STATE_HfpConnected,		SMEV_IncomingCall,				STATE_Ringing,			&Ringing);
    InitStateNode (STATE_HfpConnected,		SMEV_StartOutgoingCall,			STATE_Calling,			&OutgoingCall);
    InitStateNode (STATE_HfpConnected,		SMEV_AtResponse,				STATE_HfpConnected,		&AtProcessing);
    /*-------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: Calling   ---------------------------------------------*/
    InitStateNode (STATE_Calling,			SMEV_Disconnected,				STATE_Disconnected,		&Disconnected);
    InitStateNode (STATE_Calling,			SMEV_Failure,					STATE_HfpConnected,		&CallFailure);
    InitStateNode (STATE_Calling,			SMEV_EndCall,					STATE_HfpConnected,		&EndCall);
    InitStateNode (STATE_Calling,			SMEV_AtResponse,				&GetVoiceState1,		4);
		InitChoice (0, STATE_Calling,		SMEV_AtResponse,				STATE_InCallHeadsetOn,	&StartCall);
		InitChoice (1, STATE_Calling,		SMEV_AtResponse,				STATE_InCallHeadsetOff,	&StartCall);
		InitChoice (2, STATE_Calling,		SMEV_AtResponse,				STATE_HfpConnected,		&CallFailure);
		InitChoice (3, STATE_Calling,		SMEV_AtResponse,				STATE_Calling,			&AtProcessing);
    /*-------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: Ringing   ---------------------------------------------*/
    InitStateNode (STATE_Ringing,			SMEV_Disconnected,				STATE_Disconnected,		&Disconnected);
    InitStateNode (STATE_Ringing,			SMEV_Failure,					STATE_HfpConnected,		&EndCall);
    InitStateNode (STATE_Ringing,			SMEV_EndCall,					STATE_HfpConnected,		&EndCall);
    InitStateNode (STATE_Ringing,			SMEV_Answer,					&GetVoiceState2,		2);
		InitChoice (0, STATE_Ringing,		SMEV_Answer,					STATE_InCallHeadsetOn,	&StartCall);
		InitChoice (1, STATE_Ringing,		SMEV_Answer,					STATE_InCallHeadsetOff,	&StartCall);
    InitStateNode (STATE_Ringing,			SMEV_AtResponse,				STATE_Ringing,			&RingingCallSetup);
    /*-------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: InCallHeadsetOn   --------------------------------------*/
    InitStateNode (STATE_InCallHeadsetOn,	SMEV_Disconnected,				STATE_Disconnected,		&Disconnected);
    InitStateNode (STATE_InCallHeadsetOn,	SMEV_Failure,					STATE_HfpConnected,		&EndCallVoiceFailure);
    InitStateNode (STATE_InCallHeadsetOn,	SMEV_EndCall,					STATE_HfpConnected,		&EndCall);
    InitStateNode (STATE_InCallHeadsetOn,	SMEV_HeadsetOff,				STATE_InCallHeadsetOff,	&ToHeadsetOff);
    InitStateNode (STATE_InCallHeadsetOn,	SMEV_AtResponse,				STATE_InCallHeadsetOn,	&AtProcessing);
	InitStateNode (STATE_InCallHeadsetOn,	SMEV_SendDtmf,					STATE_InCallHeadsetOn,	&SendDtmf);
    /*--------------------------------------------------------------------------------------------------*/

    /*---------------------------------- STATE: InCallHeadsetOff  --------------------------------------*/
    InitStateNode (STATE_InCallHeadsetOff,	SMEV_Disconnected,				STATE_Disconnected,		&Disconnected);
    InitStateNode (STATE_InCallHeadsetOff,	SMEV_EndCall,					STATE_HfpConnected,		&EndCall);
    InitStateNode (STATE_InCallHeadsetOff,	SMEV_HeadsetOn,					STATE_InCallHeadsetOn,	&ToHeadsetOn);
    InitStateNode (STATE_InCallHeadsetOff,	SMEV_AtResponse,				STATE_InCallHeadsetOff,	&AtProcessing);
	InitStateNode (STATE_InCallHeadsetOff,	SMEV_SendDtmf,					STATE_InCallHeadsetOff,	&SendDtmf);
    /*--------------------------------------------------------------------------------------------------*/

	UserCallback.Construct (cb);
    HfpSmObj.Construct();
}


void HfpSm::End ()
{
    HfpSmObj.Destruct();
}


/********************************************************************************\
								SM Help functions
\********************************************************************************/
void HfpSm::StartVoice ()
{
	LogMsg ("Starting voice...");
	try
	{
		ScoAppObj->OpenSco (CurDevice->Address);
		ScoAppObj->VoiceStart();
		HeadsetOn = true;
	}
	catch (int err)
	{
		LogMsg("EXCEPTION %d", err);
		HfpSm::PutEvent_Failure2Report();	// if we have an error when starting voice, we should to notice
	}
}


void HfpSm::StopVoice ()
{
	LogMsg ("Stopping voice...");
	try
	{
		ScoAppObj->CloseSco();
		HeadsetOn = false;
	}
	catch (int err)
	{
		LogMsg("EXCEPTION %d", err);
		HfpSm::PutEvent_Failure();	// if we have an error when stopping voice, we should not to notice, it may be the normal case
	}
}

	
/********************************************************************************\
								SM TRANSITIONS
\********************************************************************************/

bool HfpSm::SelectDevice (SMEVENT* ev, int param)
{
	MyTimer.Stop();	

	uint64 addr = ev->Param.BthAddr;
	InHandDev * dev = InHand::FindDevice(addr);
	if (!dev) {
		LogMsg ("Failure to select device: %llX", addr);
		if (State == STATE_Idle)
			PutEvent_Failure();	// In order to jump back then to STATE_Idle
		goto end_func;
	}

	if (State > STATE_Disconnected)
		InHand::Disconnect ();

	LogMsg ("Selected device: %llX, %s", addr, dev->Name);
	CurDevice = dev;
	UserCallback.DevicePresent (CurDevice);
	PutEvent_ConnectStart(addr);

	end_func:
    return true;
}


bool HfpSm::ForgetDevice (SMEVENT* ev, int param)
{
	MyTimer.Stop();	
	CurDevice = 0;
	UserCallback.DeviceForgot();
    return true;
}


bool HfpSm::Disconnected (SMEVENT* ev, int param)
{
	MyTimer.Start(TIMEOUT_CONNECTION_POLLING,true);
	UserCallback.DevicePresent (CurDevice);
    return true;
}


bool HfpSm::Connect (SMEVENT* ev, int param)
{
	MyTimer.Start(TIMEOUT_CONNECTION_POLLING,true);
	InHand::BeginConnect(CurDevice->Address);
    return true;
}


bool HfpSm::Connected (SMEVENT* ev, int param)
{
	// In the case of timeout the timer is stopped because of single timer event 
	if (ev->Ev != SMEV_Timeout)
		MyTimer.Stop();	
	HfpSm::PutEvent_HfpConnectStart();
    return true;
}


bool HfpSm::HfpConnect (SMEVENT* ev, int param)
{
	// In the case HFP negotiation will not be completed in the given time,
	// we assume that it's ok and will jump to the next state
	MyTimer.Start(TIMEOUT_HFP_NEGOTIATION,true);
	AtResponsesCnt = 0;
	AtResponsesNum = InHand::BeginHfpConnect();
    return true;
}


bool HfpSm::HfpConnected (SMEVENT* ev, int param)
{
	if (ev->Ev != SMEV_Timeout)
		MyTimer.Stop();
	UserCallback.HfpConnected();
    return true;
}


bool HfpSm::ConnectFailure (SMEVENT* ev, int param)
{
	MyTimer.Start(TIMEOUT_CONNECTION_POLLING,true);
	UserCallback.ConnectFailure();
    return true;
}


bool HfpSm::ServiceConnectFailure (SMEVENT* ev, int param)
{
	UserCallback.ServiceConnectFailure();
    return true;
}


bool HfpSm::CallFailure (SMEVENT* ev, int param)
{
	UserCallback.CallFailure();
    return true;
}


bool HfpSm::IncorrectState4Call (SMEVENT* ev, int param)
{
	LogMsg ("IncorrectState4Call !!");
	UserCallback.OutgoingIncorrectState();
    return true;
}


bool HfpSm::OutgoingCall (SMEVENT* ev, int param)
{
	LogMsg ("Initiating call to: %s", ev->Param.CallNumber->Number);
	InHand::StartCall(ev->Param.CallNumber->Number);
	HeadsetOn = ev->Param.HeadsetOn;
	delete ev->Param.CallNumber;
	UserCallback.Calling();
    return true;
}


bool HfpSm::StartCall (SMEVENT* ev, int param)
{
	if (ev->Ev == SMEV_Answer)
		InHand::Answer();

	if (HeadsetOn) {
		StartVoice();
		UserCallback.InCallHeadsetOn();
	}
	else {
		UserCallback.InCallHeadsetOff();
	}
    return true;
}


bool HfpSm::EndCall (SMEVENT* ev, int param)
{
	LogMsg ("Ending call...");
	InHand::EndCall();
	if (HeadsetOn)
		StopVoice ();
	UserCallback.CallEnded();
	return true;
}


bool HfpSm::EndCallVoiceFailure (SMEVENT* ev, int param)
{
	LogMsg ("Ending call because of voice channel problem");
    bool res = EndCall (ev, param);
	if (ev->Param.ReportFailure)
		UserCallback.CallEndedVoiceFailure();
	// else it may be normal case of closing channel, so do not report
	return res;
}


bool HfpSm::ToHeadsetOn (SMEVENT* ev, int param)
{
	StartVoice();
	UserCallback.InCallHeadsetOn();
    return true;
}


bool HfpSm::ToHeadsetOff (SMEVENT* ev, int param)
{
	StopVoice();
	UserCallback.InCallHeadsetOff();
    return true;
}


bool HfpSm::Ringing (SMEVENT* ev, int param)
{
	//StartVoice(ev, param);
	UserCallback.Ring ();
    return true;
}


bool HfpSm::RingingCallSetup (SMEVENT* ev, int param)
{
	if (ev->Param.AtResponse == SMEV_AtResponse_CallSetup_None)
		PutEvent_EndCall();
    return true;
}


bool HfpSm::AtProcessing (SMEVENT* ev, int param)
{
	switch (ev->Param.AtResponse)
	{
		case SMEV_AtResponse_Ok:
		case SMEV_AtResponse_Error:
			// For STATE_HfpConnecting count the Ok response in order to generate Connected event
			if (State == STATE_HfpConnecting && (++AtResponsesCnt == AtResponsesNum)) {
				AtResponsesCnt = AtResponsesNum = 0;
				HfpSm::PutEvent_HfpConnected();
			}
			break;

		case SMEV_AtResponse_CallSetup_Incoming:
			// Treat +CIEV: 3,1 AT command as Incoming call in all states except STATE_HfpConnecting
			if (State != STATE_HfpConnecting)
				HfpSm::PutEvent_IncomingCall ();
			break;

		case SMEV_AtResponse_CallIdentity:
			// When the current state is STATE_HfpConnected, this event occurred after some failure, to ignore it
			if (State != STATE_HfpConnected)
				UserCallback.AbonentInfo(ev->Param.Abonent->Number);
			delete ev->Param.Abonent;
			break;
	}
    return true;
}


bool HfpSm::SendDtmf(SMEVENT* ev, int param)
{
	InHand::SendDtmf(&ev->Param.dtmf);
	return true;
}


/********************************************************************************\
								SM CHOICES
\********************************************************************************/

int HfpSm::GetVoiceState1 (SMEVENT* ev)
{
	// We are here: ev->Ev = SMEV_AtResponse; state = STATE_Calling
	switch (ev->Param.AtResponse)
	{
		case SMEV_AtResponse_CallSetup_Outgoing:
			return (HeadsetOn) ?   0 : 1;	// STATE_InCallHeadsetOn or STATE_InCallHeadsetOff
		case SMEV_AtResponse_CallSetup_None:
		case SMEV_AtResponse_Error:
			return 2;	// return to STATE_HfpConnected
		default:
			return 3;	// stay in STATE_Calling
	}
}


int HfpSm::GetVoiceState2 (SMEVENT* ev)
{
	// We are here: ev->Ev = SMEV_Answer; state = STATE_Ringing
	return (HeadsetOn = ev->Param.HeadsetOn) ?   0 : 1;
}
