/*******************************************************************\
 Filename   :  InHandMng.h
 Purpose    :  InTheHand.NET MANAGED C++ wrapper

 Note		:  The whole code is intentionally placed in this h-file.
               This is workaround of CLI/C++ assembly linkage problem.
			   Pls do not move it to another place.
			   Sergey K.
\*******************************************************************/

#pragma once

#include "def.h"
#include "deblog.h"
#include "DialAppType.h"
#include "HfpHelper.h"


using namespace System;
using namespace System::Text;
using namespace System::Threading;
using namespace System::Runtime::InteropServices;
using namespace System::Net::Sockets;
using namespace System::IO;
using namespace System::Windows::Forms;

using namespace InTheHand::Net;
using namespace InTheHand::Net::Sockets;
using namespace InTheHand::Net::Bluetooth;
using namespace InTheHand::Net::Bluetooth::AttributeIds;
using namespace InTheHand::Windows::Forms;


/*
 ************************************************************************************************
 C++/CLI wrapper class for InTheHand C# library.
 Its purpose to expose InTheHand's bluetooth devices SDP & RFCOMM connectivity
 For now this class is fully static
 ************************************************************************************************
 */
public ref class InHandMng
{
  public:
	static BluetoothClient^	 BthCli;		// InTheHand lib local BluetoothClient object

  public:
	static void Init();
	static void End();

	static int	GetDevices (DialAppBthDev* &devices);
	static void	FreeDevices(DialAppBthDev* &devices, int n);

	static void ClearIndicatorsNumbers();
	static void SetIndicatorsNumbers(int call, int callsetup, int callheld);

	static void BeginConnect (BluetoothAddress^ bthaddr);
	static int  BeginHfpConnect ();
	static void Disconnect ();
	static void StartCall (String ^number);
	static void SendDtmf (String^ dialchar);
	static void Answer ();
	static void EndCall ();
	static void PutOnHold();
	static void SendAtCommand (String ^at);
	static void ListCurrentCalls();

	/*
	  Handsfree Supported Features
		0	EC and/or NR function 
		1	Call waiting and three way calling; enabling is equivalent to AT+CCWA=1 - Call Waiting Notification Activation
		2	CLI presentation capability
		3	Voice recognition activation
		4	Remote volume control
		5	Enhanced call status
		6	Enhanced call control
		7	Codec negotiation
	*/
	static UInt16 HandsfreeSupportedFeatures = 116;

  protected:
	static void AddSdp(Guid svc);
	static void ProcessIoException (IOException ^ex);
	
	static void RecvAtCommand (String ^str);
	static void RecvAtCommands(array<Char> ^buf, int len);
	static void ConnectCallback (IAsyncResult ^ar);
	static void ReceiveThreadFn (Object ^state);

  protected: 
	// String^ utilities
	static char*  String2Pchar (String ^str) { return (char*) Marshal::StringToHGlobalAnsi(str).ToPointer(); }
	static void   FreePchar	(cchar *str)	 { Marshal::FreeHGlobal(IntPtr((void*)str)); }
	static wchar* String2Wchar (String ^str) { return (wchar*) Marshal::StringToHGlobalUni(str).ToPointer(); }
	static void   FreeWchar	(wchar *str)	 { Marshal::FreeHGlobal(IntPtr((void*)str)); }

	static void LogMsg (String ^str)
	{
		char * chstr = String2Pchar(str);
		InHandLog.LogMsg (chstr);
		FreePchar(chstr);
	}

  protected:
	static NetworkStream^	StreamNet;
	static StreamWriter^	StreamWtr;

  protected:
	// "+CIEV: x,y"	AT strings
	static String^	CievCall_0;
	static String^	CievCall_1;
	static String^	CievCallsetup_0;
	static String^	CievCallsetup_1;
	static String^	CievCallsetup_2;
	static String^	CievCallsetup_3;
	static String^	CievCallHeld_0;
	static String^	CievCallHeld_1;
	static String^	CievCallHeld_2;

  protected:
	static array<String^> ^CrLf = gcnew array<String^> {"\r\n"};	// for RecvAtCommands()
};


void InHandMng::Init ()
{
    try {
		AddSdp(BluetoothService::Headset  );
		AddSdp(BluetoothService::Handsfree);
		BthCli = gcnew BluetoothClient();
	}
	catch (Exception^ ex) {
		LogMsg ("EXCEPTION in InHandMng::Init: " + ex->Message);
		throw int(DialAppError_InitBluetoothRadioError);
	}
}


void InHandMng::End()
{
	BthCli  = nullptr;
}
	

void InHandMng::AddSdp (Guid svc)
{

	ServiceRecordBuilder^ bldr = gcnew ServiceRecordBuilder();
	bldr->AddServiceClass(svc);
	if(svc.Equals(BluetoothService::Handsfree))
	{
		bldr->AddServiceClass(BluetoothService::GenericAudio);
		bldr->AddCustomAttribute(gcnew ServiceAttribute(HandsFreeProfileAttributeId::SupportedFeatures,gcnew ServiceElement(ElementType::UInt16, (UInt16)HandsfreeSupportedFeatures)));
	}
		
	ServiceRecord^ sdp = bldr->ServiceRecord;

	BluetoothEndPoint ^serverEP = gcnew BluetoothEndPoint(BluetoothAddress::None, svc);
	Socket ^serverSocket = gcnew Socket(AddressFamily32::Bluetooth, SocketType::Stream, BluetoothProtocolType::RFComm);
	serverSocket->Bind(serverEP);
	serverSocket->Listen(int::MaxValue);
	byte channelNumber = static_cast<byte>((static_cast<BluetoothEndPoint^>(serverSocket->LocalEndPoint))->Port);
	ServiceRecordHelper::SetRfcommChannelNumber(sdp, channelNumber);
	System::IntPtr sdpHandle = Msft::MicrosoftSdpService::SetService(sdp->ToByteArray(), (ServiceClass)0);
	serverSocket->Close();
}


int InHandMng::GetDevices (DialAppBthDev* &devices)
{
	array<BluetoothDeviceInfo^>^	Devices;	// Paired devices list 

	Devices = BthCli->DiscoverDevices (255, false, true/*remembered*/, false, false);

	int	len = Devices->Length;
	if (len <= 0)
		return 0;

	devices = new DialAppBthDev[len];

	int	i = 0;
	for each (BluetoothDeviceInfo^ item in Devices) {
		devices[i].Address = item->DeviceAddress->ToInt64();
		devices[i].Name	   = (wchar*) Marshal::StringToHGlobalUni(item->DeviceName).ToPointer();
		i++;
	}
	return len;
}


void InHandMng::FreeDevices (DialAppBthDev* &devices, int n)
{
	if (devices) {
		for (int i=0; i<n; i++)
			Marshal::FreeHGlobal((IntPtr)((void*)devices[i].Name));

		delete[] devices;
		devices = 0;
	}
}


void InHandMng::ProcessIoException (IOException ^ex)
{
	SocketException ^sex = dynamic_cast<SocketException^>(ex->InnerException);
	if (sex) {
		SocketError Err = sex->SocketErrorCode;
		LogMsg("SocketException: " + Err.ToString() + " (" + Err.ToString("D") + ") " + ex->Message);
	}
	else {
		LogMsg("IOException: " + ex->Message);
	}
}


void InHandMng::SendAtCommand (String ^at)
{
	StreamWtr->Write(at + "\r");
	StreamWtr->Flush();
	LogMsg("HF Sent: " + at);
}


void InHandMng::ClearIndicatorsNumbers ()
{
	// Put undefined char
	CievCall_0		= "$";
	CievCall_1		= "$";
	CievCallsetup_0	= "$";
	CievCallsetup_1	= "$";
	CievCallsetup_2	= "$";
	CievCallsetup_3	= "$";
	CievCallHeld_0	= "$";
	CievCallHeld_1	= "$";
	CievCallHeld_2	= "$";
}


void InHandMng::SetIndicatorsNumbers (int call, int callsetup, int callheld)
{
	CievCall_0		= "+CIEV: " + call + ",0";
	CievCall_1		= "+CIEV: " + call + ",1";
	CievCallsetup_0	= "+CIEV: " + callsetup + ",0";		// not currently in call set up
	CievCallsetup_1	= "+CIEV: " + callsetup + ",1";		// incoming call process ongoing
	CievCallsetup_2	= "+CIEV: " + callsetup + ",2";		// outgoing call set up is ongoing
	CievCallsetup_3	= "+CIEV: " + callsetup + ",3";		// remote party being alerted in an outgoing call
	CievCallHeld_0	= "+CIEV: " + callheld + ",0";
	CievCallHeld_1	= "+CIEV: " + callheld + ",1";
	CievCallHeld_2	= "+CIEV: " + callheld + ",2";
}


void InHandMng::RecvAtCommand (String ^str)
{
	char* sinfo = String2Pchar(str);

	InHandLog.LogMsg (sinfo);

	if (str->IndexOf("OK") == 0) {
		HfpSm::PutEvent_AtResponse (SMEV_AtResponse_Ok);
	} 
	else if (str->IndexOf("ERROR") == 0) {
		HfpSm::PutEvent_AtResponse (SMEV_AtResponse_Error);
	} 
	else if (str->IndexOf("+CIND: (") == 0) {
		if (InHand::CurIndicators)
			delete InHand::CurIndicators;
		InHand::CurIndicators = new HfpIndicators();
		InHand::CurIndicators->Construct (sinfo + 7);
	} 
	else if (str->IndexOf("+CIND: ") == 0) {
		InHand::CurIndicators->SetStatuses (sinfo + 7);
		int x = InHand::CurIndicators->GetCurrentState();
		LogMsg("HfpIndicators::GetCurrentState returned " + x);
		HfpSm::PutEvent_AtResponse (SMEV_AtResponse_CurrentPhoneIndicators, x);
		delete InHand::CurIndicators;
		InHand::CurIndicators = 0;
	} 
	else if (str->IndexOf(CievCallsetup_0) == 0) {
		HfpSm::PutEvent_AtResponse(SMEV_AtResponse_CallSetup_None);
	} 
	else if (str->IndexOf(CievCallsetup_1) == 0) {
		HfpSm::PutEvent_AtResponse(SMEV_AtResponse_CallSetup_Incoming);
	}
	else if (str->IndexOf(CievCallsetup_2) == 0) {
		HfpSm::PutEvent_AtResponse(SMEV_AtResponse_CallSetup_Outgoing);
	}
	else if (str->IndexOf(CievCall_0) == 0) {
		// HfpSm::PutEvent_CallEnd();
		// In order to differ CallEnd initiated by a user and this AT command it's introduced new CallEnded event 
		// In general it is unnecessary, but because of iPhone's problem, when being in 3-way call, it stops to send
		// callsetup and callheld commands. As result we need this event in order to terminate the call
		HfpSm::PutEvent_CallEnded();
	}
	else if (str->IndexOf(CievCall_1) == 0) {
		HfpSm::PutEvent_CallStart ();
	}
	else if (str->IndexOf("+CCWA:") == 0) {
		//3-way call notification event bringing participator's number
		HfpSm::PutEvent_CallWaiting(sinfo+7);
	}
	else if (str->IndexOf("+CLIP:") == 0) {
		HfpSm::PutEvent_AtResponse (SMEV_AtResponse_CallingLineId, sinfo+7);
	}
	else if (str->IndexOf("+CLCC:") == 0) {
		HfpSm::PutEvent_AtResponse (SMEV_AtResponse_ListCurrentCalls, sinfo+7);
	}
	else if (str->IndexOf(CievCallHeld_0) == 0) {
		HfpSm::PutEvent_CallHeld (SMEV_AtResponse_CallHeld_None);
	}
	else if (str->IndexOf(CievCallHeld_1) == 0) {
		HfpSm::PutEvent_CallHeld (SMEV_AtResponse_CallHeld_HeldAndActive);
	}
	else if (str->IndexOf(CievCallHeld_2) == 0) {
		HfpSm::PutEvent_CallHeld (SMEV_AtResponse_CallHeld_HeldOnly);
	}

	/* 
	This works on HTC-Diamond2 - Windows Mobile only
	else if (str->IndexOf("+COLP") == 0) { 
		int i1 = str->IndexOf('"');
		int i2 = str->LastIndexOf('"');
		if (i1 > 0  &&  i2 > i1) {
			String ^s = str->Substring (i1+1, i2-i1-1);
			wchar* swinfo = String2Wchar(s);
			HfpSm::PutEvent_AtResponse (SMEV_AtResponse_CallIdentity, swinfo);
			FreeWchar(swinfo);
		}
	}
	*/

	FreePchar(sinfo);
}


void InHandMng::RecvAtCommands (array<Char> ^buf, int len)
{
	String ^str = gcnew String (buf, 0, len);
	array<String^> ^cmds = str->Split (CrLf, StringSplitOptions::RemoveEmptyEntries);
	// str leading <cr><lf> is also skipped

	for each (String^ s in cmds)
		RecvAtCommand (s);
}


void InHandMng::ReceiveThreadFn (Object ^state)
{
	ASSERT_ (StreamNet->CanRead);

	StreamReader ^rdr = gcnew StreamReader(StreamNet, Encoding::ASCII);
	array<Char>  ^buf = gcnew array<Char>(250);
	try	{
		while (true)
		{
			// We don't use ReadLine because we then don't get to see the CR/LF. 
			// And we often get the series \r\r\n, which should appear as one new line.
			int nread = rdr->Read(buf, 0, buf->Length);
			if (nread == 0) {
				InHandLog.LogMsg ("ReceiveThreadFn detected disconnection");
				break;
			}
			RecvAtCommands (buf, nread);
		}
	}
	catch (IOException ^ex) {
		ProcessIoException (ex);
	}
	catch (Exception ^ex) {
		LogMsg(ex->Message);
	}
	HfpSm::PutEvent_Disconnect();
}


#if 0
// Because of the problem in both 32feet's SelectBluetoothDeviceDialog and native BluetoothSelectDevices func,
// (when fAddNewDeviceWizard = FALSE), it was implemented in DialApp.cpp - dialappBluetoothSelectDevice()
// In the future the dialappBluetoothSelectDevice func will be used
uint64 InHandMng::UiSelectDevice ()
{
	SelectBluetoothDeviceDialog^ dlg = gcnew SelectBluetoothDeviceDialog();
	DialogResult rslt = dlg->ShowDialog();
	if (rslt != DialogResult->OK) {
		return 0;
	}
	return dlg->SelectedDevice->DeviceAddress->ToInt64();
}
#endif


void InHandMng::BeginConnect (BluetoothAddress^ bthaddr)
{
	try	{
		AsyncCallback^ cbk = gcnew AsyncCallback (&ConnectCallback);
		BthCli->BeginConnect(bthaddr, BluetoothService::Handsfree, cbk, nullptr);
	}
	catch (IOException ^ex) {
		ProcessIoException (ex);
		HfpSm::PutEvent_Disconnect();
	}
	catch (Exception ^ex) {
		LogMsg(ex->Message);
		HfpSm::PutEvent_Failure(DialAppError_ConnectFailure);
	}
}


void InHandMng::ConnectCallback (IAsyncResult ^ar)
{
	LogMsg ("InHandMng::ConnectCallback");
	try
	{
		BthCli->EndConnect(ar);
		StreamNet = BthCli->GetStream();
		StreamWtr = gcnew StreamWriter (StreamNet, Encoding::ASCII);
		HfpSm::PutEvent_Connected();
		ThreadPool::QueueUserWorkItem(gcnew WaitCallback(ReceiveThreadFn));
	}
	catch (IOException ^ex) {
		ProcessIoException (ex);
		HfpSm::PutEvent_Disconnect();
	}
	catch (Exception ^ex) {
		LogMsg(ex->Message);
		HfpSm::PutEvent_Failure(DialAppError_ConnectFailure);
	}
}


int InHandMng::BeginHfpConnect ()
{
	try
	{
		SendAtCommand("AT+BRSF=" + HandsfreeSupportedFeatures); // Used In HF SDP, according to HF Spec 4.2.1
		SendAtCommand("AT+CIND=?");			// The mapping of the indicators 
		SendAtCommand("AT+CMER=3,0,0,1");	// Indicators status update: 3,0,0,1 activates "indicator events reporting".
		SendAtCommand("AT+CMEE=1");			// Enable the use of result code +CME ERROR
		SendAtCommand("AT+CCWA=1");			// Call Waiting Notification Activation
		SendAtCommand("AT+CLIP=1");			// Calling Line Identification notification HF Spec 4.23 (sending incoming call info along with RING)
		SendAtCommand("AT+CIND?");			// Get current indicators
		return 7; // number of sent AT commands
	}
	catch (IOException ^ex) {
		ProcessIoException (ex);
		HfpSm::PutEvent_Failure (DialAppError_ServiceConnectFailure);
	}
	catch (Exception ^ex) {
		LogMsg(ex->Message);
		HfpSm::PutEvent_Failure (DialAppError_ServiceConnectFailure);
	}
	return 0;
}


void InHandMng::Disconnect ()
{
	try	{
		if (StreamWtr) {
			StreamWtr->Close();
			StreamWtr = nullptr;
		}
		if (StreamNet) {
			StreamNet->Close();
			StreamNet = nullptr;
		}
		BthCli->Close();
	}
	catch (Exception ^ex) {
		LogMsg(ex->Message);
		//Do not generate Failure event when disconnecting - the client can be already disconnected
		//HfpSm::PutEvent_Failure (DialAppError_ConnectFailure);
	}

	try	{
		// Recreate BthCli because of there is no Open method
		BthCli = gcnew BluetoothClient();
	}
	catch (Exception ^ex) {
		LogMsg(ex->Message);
		HfpSm::PutEvent_Failure (DialAppError_InternalError);
	}
}


void InHandMng::StartCall(String^ number)
{
	try	{
		//SendAtCommand("AT+BLDN"); - redial last
		SendAtCommand("ATD" + number + ";");
	}
	catch (IOException ^ex) {
		ProcessIoException (ex);
		HfpSm::PutEvent_Disconnect();
	}
	catch (Exception ^ex) {
		LogMsg(ex->Message);
		HfpSm::PutEvent_Failure (DialAppError_CallFailure);
	}
}


void InHandMng::SendDtmf(String^ dialchar)
{
	try	{
		SendAtCommand("AT+VTS=" + dialchar + ";");
	}
	catch (IOException ^ex) {
		ProcessIoException (ex);
		HfpSm::PutEvent_Disconnect();
	}
	catch (Exception ^ex) {
		LogMsg(ex->Message);
		HfpSm::PutEvent_Failure (DialAppError_ConnectFailure);
	}
}


void InHandMng::Answer()
{
	try	{
		SendAtCommand("ATA");
	}
	catch (IOException ^ex) {
		ProcessIoException (ex);
		HfpSm::PutEvent_Disconnect();
	}
	catch (Exception ^ex) {
		LogMsg(ex->Message);
		HfpSm::PutEvent_Failure (DialAppError_ConnectFailure);
	}
}


void InHandMng::EndCall()
{
	try	{
		SendAtCommand("AT+CHUP");	// terminating a ongoing single call or a held call.
		SendAtCommand("ATH");		// HTC-Diamond2 terminates by old-style command only
	}
	catch (IOException ^ex) {
		ProcessIoException (ex);
		HfpSm::PutEvent_Disconnect();
	}
	catch (Exception ^ex) {
		LogMsg(ex->Message);
		HfpSm::PutEvent_Failure (DialAppError_ConnectFailure);
	}
}

/*
AT+CHLD=? +CHLD: (list of supported <n>s)
AT+CHLD=[<n>]
<n>: (integer type)
0 releases all held calls or sets User Determined User Busy (UDUB) for a
waiting call
1 releases all active calls (if any exist) and accepts the other (held or waiting)
call
1x releases a specific active call X
2 places all active calls (if any exist) on hold and accepts the other (held or
waiting) call
2x places all active calls on hold except call X with which communication shall
be supported
3 adds a held call to the conversation
4 connects the two calls and disconnects the subscriber from both calls
(ECT)
*/
void InHandMng::PutOnHold()
{	
	SendAtCommand("AT+CHLD=2;");
}


void InHandMng::ListCurrentCalls()
{
	try	{
		SendAtCommand("AT+CLCC;");
	}
	catch (IOException ^ex) {
		ProcessIoException (ex);
		HfpSm::PutEvent_Disconnect();
	}
	catch (Exception ^ex) {
		LogMsg(ex->Message);
		HfpSm::PutEvent_Failure (DialAppError_ConnectFailure);
	}
}
