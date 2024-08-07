#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <InitGuid.h>
#include <unordered_map>
#include <psapi.h>
#include <Memoryapi.h>
#include <tlhelp32.h>
#include <set>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

//extern "C"
//{
//#include "C:/Users/Jonni/source/repos/luajitGLWrapper/OpenGLProject/luajit210b3/include/lua.h"
//#include "C:/Users/Jonni/source/repos/luajitGLWrapper/OpenGLProject/luajit210b3/include/lauxlib.h"
//#include "C:/Users/Jonni/source/repos/luajitGLWrapper/OpenGLProject/luajit210b3/include/lualib.h"
//}
//
////#pragma comment(lib, "C:/Users/Jonni/Desktop/LuaJIT-2.1.0-beta3/src/lua51.lib")
//#pragma comment(lib, "Q:/shitcans/doodoocan7/_LuaJIT-2.1.0-beta3/src/lua51.lib")
BOOL CALLBACK EnumFFDevicesCallback(const DIDEVICEINSTANCE* pInst, VOID* pContext);
BOOL CALLBACK EnumAxesCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext);
VOID FreeDirectInput();
bool F5Held = false;
HRESULT SetDeviceForcesXY();
DIJOYSTATE idk = {};
bool ModActive = false;
LPDIRECTINPUT8          g_pDI = nullptr;
LPDIRECTINPUTDEVICE8    g_pDevice = nullptr;
LPDIRECTINPUTEFFECT     g_pEffect = nullptr;
BOOL                    g_bActive = TRUE;
DWORD                   g_dwNumForceFeedbackAxis = 0;
INT                     g_nXForce = 0;
INT                     g_nYForce = 0;
HWND					HWNDProgram = nullptr;
#define M_PI			3.14159265358979323846
#define HalfPI          1.5707963267949
#define PI2				6.2831853071796

struct Vector3
{
	float x;
	float z;
	float y;
};

class Entity
{
public:
	Vector3 Angle; //0x0000
	char pad_000C[36];
	Vector3 Position; //0x0030
	char pad_003C[148];
	Vector3 Velocity; //0x00D0
	char pad_0D8[0x7F4];
	float LeftTireAngle;
	char pad_3B0[0x3AC];
	float RightTireAngle;
};
Entity* MyCar = {};

#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=nullptr; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=nullptr; } }
//-----------------------------------------------------------------------------
// Name: InitDirectInput()
// Desc: Initialize the DirectInput variables.
//-----------------------------------------------------------------------------
std::string formatError(HRESULT hr) {
	switch (hr) {
	case DI_OK:                     return "DI_OK";
	case DIERR_INVALIDPARAM:        return "DIERR_INVALIDPARAM";
	case DIERR_NOTINITIALIZED:      return "DIERR_NOTINITIALIZED";
	case DIERR_ALREADYINITIALIZED:  return "DIERR_ALREADYINITIALIZED";
	case DIERR_INPUTLOST:           return "DIERR_INPUTLOST";
	case DIERR_ACQUIRED:            return "DIERR_ACQUIRED";
	case DIERR_NOTACQUIRED:         return "DIERR_NOTACQUIRED";
	case E_HANDLE:                  return "E_HANDLE";
	case DIERR_DEVICEFULL:          return "DIERR_DEVICEFULL";
	case DIERR_DEVICENOTREG:        return "DIERR_DEVICENOTREG";
	default:                        return "UNKNOWN";
	}
}


HRESULT InitDirectInput(HWND fuck)
{
	DIPROPDWORD dipdw;
	HRESULT hr;

	//g_pDevice->Unacquire();

	// Register with the DirectInput subsystem and get a pointer
	// to a IDirectInput interface we can use.

	if (FAILED(hr = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION,
		IID_IDirectInput8, (VOID**)&g_pDI, nullptr)))
	{
		printf("DirectInput8Create failed\n");
		return hr;
	}

	// Look for a force feedback device we can use

	if (FAILED(hr = g_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL,
		EnumFFDevicesCallback, nullptr,
		DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK)))
	{
		printf("g_pDI->EnumDevices failed\n");
		return hr;
	}

	if (!g_pDevice)
	{
		printf("[C++] No DI Device !g_pDevice Line 112\n");
		return 420;
	}
	//g_pDevice->GetDeviceInfo();

	// Set the data format to "simple joystick" - a predefined data format. A
	// data format specifies which controls on a device we are interested in,
	// and how they should be reported.
	//
	// This tells DirectInput that we will be passing a DIJOYSTATE structure to
	// IDirectInputDevice8::GetDeviceState(). Even though we won't actually do
	// it in this sample. But setting the data format is important so that the
	// DIJOFS_* values work properly.
	if (FAILED(hr = g_pDevice->SetDataFormat(&c_dfDIJoystick)))
	{
		printf("g_pDevice->SetDataFormat failed\n");
		return hr;
	}


	// Set the cooperative level to let DInput know how this device should
	// interact with the system and with other DInput applications.
	// Exclusive access is required in order to perform force feedback.
	if (FAILED(hr = g_pDevice->SetCooperativeLevel(fuck, DISCL_EXCLUSIVE | DISCL_FOREGROUND)))
	{
		printf("g_pDevice->SetCooperativeLevel failed\n");
		return hr;
	}

	// Since we will be playing force feedback effects, we should disable the
	// auto-centering spring.
	dipdw.diph.dwSize = sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj = 0;
	dipdw.diph.dwHow = DIPH_DEVICE;
	dipdw.dwData = FALSE;
	if (FAILED(hr = g_pDevice->SetProperty(DIPROP_AUTOCENTER, &dipdw.diph)))
	{
		return hr;
		printf("g_pDevice->SetProperty failed\n");
	}


	// Enumerate and count the axes of the joystick 
	if (FAILED(hr = g_pDevice->EnumObjects(EnumAxesCallback,
		(VOID*)&g_dwNumForceFeedbackAxis, DIDFT_AXIS)))
		return hr;

	// This simple sample only supports one or two axis joysticks
	g_dwNumForceFeedbackAxis = 1;


	//if (g_dwNumForceFeedbackAxis > 2)
	//	g_dwNumForceFeedbackAxis = 2;

	// This application needs only one effect: Applying raw forces.
	DWORD rgdwAxes[1] = { DIJOFS_X };
	LONG rglDirection[1] = { 0 };
	DICONSTANTFORCE cf = { 0 };

	DIEFFECT eff;
	ZeroMemory(&eff, sizeof(eff));
	eff.dwSize = sizeof(DIEFFECT);
	eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
	eff.dwDuration = 50000;
	eff.dwSamplePeriod = 0;
	eff.dwGain = DI_FFNOMINALMAX;
	eff.dwTriggerButton = DIEB_NOTRIGGER;
	eff.dwTriggerRepeatInterval = 0;
	eff.cAxes = g_dwNumForceFeedbackAxis;
	eff.rgdwAxes = rgdwAxes;
	eff.rglDirection = rglDirection;
	eff.lpEnvelope = 0;
	eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
	eff.lpvTypeSpecificParams = &cf;
	eff.dwStartDelay = 0;
	// Create the prepared effect
	if (FAILED(hr = g_pDevice->CreateEffect(GUID_ConstantForce,
		&eff, &g_pEffect, nullptr)))
	{
		printf("g_pDevice->CreateEffect failed\n");
		return hr;
	}

	if (!g_pEffect)
	{
		printf("!g_pEffect\n");
		return E_FAIL;
	}


	return S_OK;
}


//-----------------------------------------------------------------------------
// Name: EnumAxesCallback()
// Desc: Callback function for enumerating the axes on a joystick and counting
//       each force feedback enabled axis
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumAxesCallback(const DIDEVICEOBJECTINSTANCE* pdidoi,
	VOID* pContext)
{
	auto pdwNumForceFeedbackAxis = reinterpret_cast<DWORD*>(pContext);

	if ((pdidoi->dwFlags & DIDOI_FFACTUATOR) != 0)
		(*pdwNumForceFeedbackAxis)++;

	return DIENUM_CONTINUE;
}




//-----------------------------------------------------------------------------
// Name: EnumFFDevicesCallback()
// Desc: Called once for each enumerated force feedback device. If we find
//       one, create a device interface on it so we can play with it.
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumFFDevicesCallback(const DIDEVICEINSTANCE* pInst,
	VOID* pContext)
{
	LPDIRECTINPUTDEVICE8 pDevice;
	HRESULT hr;

	// Obtain an interface to the enumerated force feedback device.

	hr = g_pDI->CreateDevice(GUID_Joystick, &pDevice, nullptr);
	//hr = g_pDI->CreateDevice(pInst->guidInstance, &pDevice, nullptr);

	//pInst->guidInstance
	// If it failed, then we can't use this device for some
	// bizarre reason.  (Maybe the user unplugged it while we
	// were in the middle of enumerating it.)  So continue enumerating
	if (FAILED(hr))
		return DIENUM_CONTINUE;

	// We successfully created an IDirectInputDevice8.  So stop looking 
	// for another one.
	g_pDevice = pDevice;
	pDevice = nullptr;

	return DIENUM_STOP;
}

//-----------------------------------------------------------------------------
// Name: FreeDirectInput()
// Desc: Initialize the DirectInput variables.
//-----------------------------------------------------------------------------
VOID FreeDirectInput()
{
	// Unacquire the device one last time just in case 
	// the app tried to exit while the device is still acquired.
	if (g_pDevice)
		g_pDevice->Unacquire();

	// Release any DirectInput objects.
	SAFE_RELEASE(g_pEffect);
	SAFE_RELEASE(g_pDevice);
	SAFE_RELEASE(g_pDI);
}



//-----------------------------------------------------------------------------
// Name: SetDeviceForcesXY()
// Desc: Apply the X and Y forces to the effect we prepared.
//-----------------------------------------------------------------------------
HRESULT SetDeviceForcesXY() 
{
	// Modifying an effect is basically the same as creating a new one, except
	// you need only specify the parameters you are modifying
	LONG rglDirection;
	DICONSTANTFORCE cf;


	cf.lMagnitude = g_nXForce;
	rglDirection = 0;

	//else
	//{
	//	// If two force feedback axis, then apply magnitude from both directions 
	//	rglDirection[0] = g_nXForce;
	//	rglDirection[1] = g_nYForce;
	//	cf.lMagnitude = (DWORD)sqrt((double)g_nXForce * (double)g_nXForce +
	//		(double)g_nYForce * (double)g_nYForce);
	//}

	//printf("[C++] g_dwNumForceFeedbackAxis %d\n", g_dwNumForceFeedbackAxis);
	//HRESULT hr;
	//if (FAILED(hr = InitDirectInput()))
	//{
	//	printf("[C++] InitDirectInput failed Errorcode: %d, Error message: %s\n", hr, formatError(hr).c_str());
	//}
	DIEFFECT eff;
	ZeroMemory(&eff, sizeof(eff));
	eff.dwSize = sizeof(DIEFFECT);
	eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
	eff.cAxes = g_dwNumForceFeedbackAxis;
	eff.rglDirection = &rglDirection;
	eff.lpEnvelope = 0;
	eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
	eff.lpvTypeSpecificParams = &cf;
	eff.dwStartDelay = 0;
	//std::cout << g_dwNumForceFeedbackAxis << std::endl;
	// Now set the new parameters and start the effect immediately.
	return g_pEffect->SetParameters(&eff, DIEP_DIRECTION |
		DIEP_TYPESPECIFICPARAMS |
		DIEP_START);
}


DWORD Base = (DWORD)GetModuleHandle("Fouc.exe");
DWORD Ptrs[5] = { 0x8E98FC0,4,0,0xC,0x18 };

Entity* GetVehicleEntity()
{
	DWORD Tmp = Base;
	for (int i = 0; i < 5; i++)
	{
		Tmp += Ptrs[i];
		Tmp = *(int*)Tmp;
		if (Tmp < Base) {
			return 0;
		}
	}
	return (Entity*)Tmp;
}

bool AcquireDevice()
{
	HRESULT hr;
	if (FAILED(hr = g_pDevice->Acquire()))
	{
		printf("[C++] g_pDevice->Acquire failed Errorcode: %d, Error message: %s\n", hr, formatError(hr).c_str());
		return false;
	}
	else
	{
		return true;
	}
}


void RunFFB()
{
	float velAng = atan2(MyCar->Velocity.x, MyCar->Velocity.y);
	float Speed = sqrt(MyCar->Velocity.x * MyCar->Velocity.x + MyCar->Velocity.y * MyCar->Velocity.y + MyCar->Velocity.z * MyCar->Velocity.z);
	float carAng = atan2(MyCar->Angle.x, MyCar->Angle.y);

	float	FFBValue = FFBValue = fmod(velAng - carAng + HalfPI, PI2);
			FFBValue = fmod(FFBValue + PI2, PI2);
			FFBValue -= (MyCar->LeftTireAngle+MyCar->RightTireAngle) / 2.f;
			//8D0(2256), C80(3200)

	if (FFBValue > M_PI)
	{
		FFBValue -= PI2;
	}

	if (Speed < 5.0f || abs(FFBValue > 0.8f))
	{
		FFBValue = 0.000001f;

	}
	FFBValue = FFBValue * 60000.f;

	FFBValue = min(6000.f, FFBValue);
	FFBValue = max(-6000.f, FFBValue);
	g_nXForce = (int)-FFBValue;
	SetDeviceForcesXY();
}


void InitMod()
{
	FreeDirectInput();
	InitDirectInput(GetForegroundWindow());
	AcquireDevice();
	
	MyCar = GetVehicleEntity();

	if ((int)MyCar > 0)
	{
		printf("Reinit ffb\nDebug info:\n");
		printf("carptr %p\n", MyCar);
		printf("angle %f %f\n", MyCar->Angle.x, MyCar->Angle.y);
		printf("pos %f %f %f\n", MyCar->Position.x, MyCar->Position.y, MyCar->Position.z);
		printf("vel %f %f %f\n", MyCar->Velocity.x, MyCar->Velocity.y, MyCar->Velocity.z);
		printf("left tire %f %p\nright tire %f %p\n", MyCar->LeftTireAngle,&MyCar->LeftTireAngle,MyCar->RightTireAngle,&MyCar->RightTireAngle);
	}
	else
	{
		printf("Reinit ffb\nDebug info: No vehicle!\n");
	}
	
}


DWORD WINAPI MainThread(LPVOID param) {
	if (AllocConsole())
	{
		freopen("CONOUT$", "w", stdout);
	}
	printf("F5 = load/reload the mod\n");


	while (true) {

		if (GetAsyncKeyState(VK_RSHIFT) && GetAsyncKeyState(VK_ESCAPE)) 
		{
			FreeDirectInput();
			break;
		}

		if (!F5Held && GetAsyncKeyState(VK_F5))
		{
			InitMod();
			ModActive = true;
		}
		if (ModActive)
		{ 
			if (AcquireDevice())
			{
				MyCar = GetVehicleEntity();
				//printf("%p WDJAIOAWJO\n", (int)MyCar);
				if ((int)MyCar > 0)
				{
					RunFFB();
				}
			
			}
			else
			{
				Sleep(400);
			}
		}
		F5Held = GetAsyncKeyState(VK_F5);
		Sleep(10);
	}
	printf("Releasing handle\n");
	FreeLibraryAndExitThread((HMODULE)param, 0);
	return 0;
}


BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved) {
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		CreateThread(0, 0, MainThread, hModule, 0, 0);
		break;
	}
	return TRUE;
}
