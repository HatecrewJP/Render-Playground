#include <windows.h>
#include <stdio.h>
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
		unsigned RepeatCount : 15;
		unsigned ScanCode : 7;
		unsigned IsExtended : 1;
		unsigned Reserved : 2;
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
				printf("%u\n",State.RepeatCount);

			}break;
			
			case WM_QUIT: {
				Running = 0;
			}break;
			}
			DispatchMessageA(&CurrentMessage);
		}

	}

}