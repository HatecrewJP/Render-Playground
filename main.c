#include <windows.h>
#include <stdio.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#define DEBUG 1

#define ArrayCount(x) (sizeof(x)/sizeof((x)[0]))

typedef unsigned char byte;
typedef int bool;


#define ASSERT(x) if(!(x)) *(char*)0=0;

LRESULT Wndproc(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam){
	LRESULT Result = 0;
	switch (Message) {
	case WM_CLOSE: {
		PostQuitMessage(0);
		break;
	}
	case WM_DESTROY: {
		PostQuitMessage(0);
		break;
	}
	default: {
		Result = DefWindowProcA(WindowHandle, Message, WParam, LParam);
		break;
	}
	}

	return Result;
}


typedef union KeyDownState {
	unsigned Value;
	struct {
		unsigned RepeatCount : 16;
		unsigned ScanCode : 8;
		unsigned IsExtended : 1;
		unsigned Reserved : 4;
		unsigned ContextCode : 1;
		unsigned PreviousState : 1;
		unsigned TransitionState : 1;
	};
}KeyDownState;

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	FILE* fp;
	ASSERT(AllocConsole());
	freopen_s(&fp, "CONIN$", "r", stdin);
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);

	WNDCLASSA WindowClass;
	WindowClass.style = CS_HREDRAW | CS_OWNDC | CS_VREDRAW;
	WindowClass.lpfnWndProc = Wndproc;
	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;
	WindowClass.hInstance = hInst;
	WindowClass.hIcon = NULL;
	WindowClass.hCursor = NULL;
	WindowClass.hbrBackground = NULL;
	WindowClass.lpszMenuName = NULL;
	WindowClass.lpszClassName = "ACalculator";

	ATOM WindowAtom = RegisterClassA(&WindowClass);
	ASSERT(WindowAtom);

	HWND WindowHandle = CreateWindowExA(
		0, 
		WindowClass.lpszClassName, 
		"ACalculator", 
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
		CW_USEDEFAULT, 
		CW_USEDEFAULT, 
		CW_USEDEFAULT, 
		CW_USEDEFAULT, 
		NULL, 
		NULL, 
		hInst, 
		NULL);

	if(!WindowHandle){
#ifdef DEBUG
		ASSERT(0);
#endif 
		exit(-1);
	}

	UINT DeviceFlags = 0;
#ifdef DEBUG
	DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // DEBUG

	D3D_FEATURE_LEVEL FeatureLevels[1] = {D3D_FEATURE_LEVEL_11_0};

	RECT rc;
	ASSERT(GetClientRect(WindowHandle, &rc));
	UINT RectWidth = rc.right - rc.left;
	UINT RectHeight = rc.bottom - rc.top;




	ID3D11Device *Device = NULL;
	ID3D11DeviceContext *DeviceContext = NULL;

	UINT FeatureLevelCount = (sizeof(FeatureLevels) / sizeof((FeatureLevels)[0]));
	HRESULT res = D3D11CreateDevice(NULL, D3D10_DRIVER_TYPE_WARP, 0, DeviceFlags, FeatureLevels, ArrayCount(FeatureLevels), D3D11_SDK_VERSION,&Device,NULL,&DeviceContext);
	if(res != S_OK){
		ASSERT(0);
		exit(-1);
	}
	ASSERT(Device!=NULL && DeviceContext);

	IDXGIDevice1 *DXGIDevice = NULL;
	res = Device->lpVtbl->QueryInterface(Device, &IID_IDXGIDevice1, &DXGIDevice);
	if(res != S_OK){
		ASSERT(0);
	}
	

	bool Running = 1;
	while (Running) {
		MSG CurrentMessage;
		while (PeekMessageA(&CurrentMessage, 0, 0, 0, PM_REMOVE)) {
			UINT MessageIdentifier = CurrentMessage.message;
			unsigned WParam = (unsigned)CurrentMessage.wParam;
			unsigned LParam = (unsigned) CurrentMessage.lParam;
			switch (MessageIdentifier){
			case WM_KEYDOWN: {
				KeyDownState State;
				State.Value = (unsigned)LParam;
				printf("%u\n",State.ScanCode);

			}break;
			
			case WM_QUIT: {
				Running = 0;
			}break;
			}
			DispatchMessageA(&CurrentMessage);
		}

	}

}