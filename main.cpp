#include "Ix64.h"


#include <windows.h>
#include <stdio.h>
#include <math.h>


#define ArrayCount(x) (sizeof(x)/sizeof((x)[0]))
#define ToBit(x) ((x)*8)


#include "Application.cpp"
struct Win32ColorBGRA{
	unsigned char B;
	unsigned char G;
	unsigned char R;
	unsigned char A;
};

struct Win32FrameBuffer{
	unsigned Width;
	unsigned Height;
	unsigned PixelSize;
	Win32ColorBGRA **Pixels;
	BITMAPINFO BMPInfo;
};

struct Win32BackBuffer{
	UINT Width;
	UINT Height;
	Win32ColorBGRA *Data;
	UINT Stride;
	WORD PixelSize;
};
global HDC GlobalDeviceContext = NULL;
global bool NeedsResize = 0;

struct Win32KeyDownState{
	unsigned RepeatCount : 16;
	unsigned ScanCode : 8;
	unsigned IsExtended : 1;
	unsigned Reserved : 4;
	unsigned ContextCode : 1;
	unsigned PreviousState : 1;
	unsigned TransitionState : 1;
};



Win32FrameBuffer GlobalFrameBuffer;

internal void Win32DisplayBuffer(HWND WindowHandle, HDC DeviceContext, Win32FrameBuffer *Buffer){
	ASSERT(WindowHandle);
	ASSERT(DeviceContext);
	ASSERT(Buffer);

	RECT rc;
	ASSERT(GetClientRect(WindowHandle, &rc));
	UINT RectWidth = rc.right - rc.left;
	UINT RectHeight = rc.bottom - rc.top;

	BITMAPINFO BMPInfo = Buffer->BMPInfo;
	UINT BufferWidth = Buffer->Width;
	UINT BufferHeight = Buffer->Height;
	void *BufferData = *(Buffer->Pixels);
	SetDIBitsToDevice(DeviceContext, 0, 0, BufferWidth, BufferHeight, 0, 0, 0, BufferHeight, BufferData, &BMPInfo, DIB_RGB_COLORS);
}



internal void Win32ResizeFrameBuffer(struct Win32FrameBuffer *FrameBuffer, struct Win32BackBuffer *BackBuffer, UINT NewWidth, UINT NewHeight){
	if((NewWidth > BackBuffer->Width) || (NewHeight > BackBuffer->Height)){
		if(BackBuffer->Data){
			VirtualFree(BackBuffer->Data, 0, MEM_RELEASE);
			BackBuffer->Data = NULL;
		}
		BackBuffer->Width = NewWidth;
		BackBuffer->Height = NewHeight;
		BackBuffer->Data = (Win32ColorBGRA *)VirtualAlloc(NULL, BackBuffer->Width * BackBuffer->Height * BackBuffer->PixelSize, MEM_COMMIT, PAGE_READWRITE);
		ASSERT(BackBuffer->Data);
	}

	FrameBuffer->Width = NewWidth;
	FrameBuffer->Height = NewHeight;
	FrameBuffer->BMPInfo.bmiHeader.biWidth = FrameBuffer->Width;
	FrameBuffer->BMPInfo.bmiHeader.biHeight = -(INT)FrameBuffer->Height;
}

LRESULT Wndproc(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam){
	LRESULT Result = 0;
	switch (Message) {
	case WM_SIZE:{
		NeedsResize = 1;
		break;
	}
	case WM_CLOSE: {
		PostQuitMessage(0);
		break;
	}
	case WM_DESTROY: {
		PostQuitMessage(0);
		break;
	}
	case WM_PAINT: {
		printf("WM_PAINT\n");
		PAINTSTRUCT PaintStruct;
		HDC DeviceContext = BeginPaint(WindowHandle, &PaintStruct);
		Win32DisplayBuffer(WindowHandle, DeviceContext,&GlobalFrameBuffer);

		EndPaint(WindowHandle, &PaintStruct);
		
	}break;
	default: {
		Result = DefWindowProcA(WindowHandle, Message, WParam, LParam);
		break;
	}
	
	}

	return Result;
}





internal void Win32ClearBackBuffer(struct Win32FrameBuffer *Buffer){
	ASSERT(Buffer);
	ASSERT(Buffer->Pixels);
	ASSERT(*(Buffer->Pixels))

	void *Data = *(Buffer->Pixels);
	UINT Size = Buffer->Width * Buffer->Height * (UINT)Buffer->PixelSize;
	memset(Data, 0, Size);
}



#include "DebugWin32.cpp"
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
	WindowClass.lpszClassName = "Window";

	ATOM WindowAtom = RegisterClassA(&WindowClass);
	ASSERT(WindowAtom);

	HWND WindowHandle = CreateWindowExA(
		0,
		WindowClass.lpszClassName, 
		"Window", 
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
		0, 
		0, 
		1920, 
		1080, 
		NULL, 
		NULL, 
		hInst, 
		NULL);

	GlobalDeviceContext = GetDC(WindowHandle);
#ifdef DEBUG
	DebugWindowHandle = WindowHandle;
	DebugDeviceContext = GlobalDeviceContext;
#endif
	
	if(!WindowHandle){
#ifdef DEBUG
		
#endif 
		exit(-1);
	}
	
	RECT CurrentRect;
	ASSERT(GetClientRect(WindowHandle, &CurrentRect));
	
	HDC ScreenDeviceContext = GetDC(NULL);
	ASSERT(ScreenDeviceContext);



	Win32BackBuffer BackBuffer = {};
	BackBuffer.Width  = CurrentRect.right - CurrentRect.left;
	BackBuffer.Height = CurrentRect.bottom - CurrentRect.top;
	BackBuffer.PixelSize = sizeof(Win32ColorBGRA);
	BackBuffer.Data = (Win32ColorBGRA*)VirtualAlloc(NULL, BackBuffer.Width * BackBuffer.Height * BackBuffer.PixelSize, MEM_COMMIT, PAGE_READWRITE);
	BackBuffer.Stride = BackBuffer.Width * BackBuffer.PixelSize;
	ASSERT(BackBuffer.Data);

	GlobalFrameBuffer.BMPInfo.bmiHeader.biSize = sizeof(GlobalFrameBuffer.BMPInfo.bmiHeader);
	GlobalFrameBuffer.BMPInfo.bmiHeader.biWidth = BackBuffer.Width;
	GlobalFrameBuffer.BMPInfo.bmiHeader.biHeight = -(INT)BackBuffer.Height;
	GlobalFrameBuffer.BMPInfo.bmiHeader.biPlanes = 1;
	GlobalFrameBuffer.BMPInfo.bmiHeader.biBitCount = ToBit(BackBuffer.PixelSize);
	GlobalFrameBuffer.BMPInfo.bmiHeader.biCompression = BI_RGB;

	GlobalFrameBuffer.PixelSize = BackBuffer.PixelSize;

	GlobalFrameBuffer.Height = BackBuffer.Height;
	GlobalFrameBuffer.Width = BackBuffer.Width;
	GlobalFrameBuffer.Pixels = &BackBuffer.Data;

#ifdef DEBUG
	DebugFrameBuffer = &GlobalFrameBuffer;
#endif

	bool Running = 1;

	while (Running) {
		MSG CurrentMessage;

		while (PeekMessageA(&CurrentMessage, 0, 0, 0, PM_REMOVE)) {
			UINT MessageIdentifier = CurrentMessage.message;
			unsigned long WParam = (unsigned long) CurrentMessage.wParam;
			unsigned long LParam = (unsigned long) CurrentMessage.lParam;
			

			switch (MessageIdentifier){
			case WM_KEYDOWN: {
				Win32KeyDownState State;
				ASSERT(sizeof(State)==sizeof(LParam))
				memcpy(&State, &LParam, sizeof(State));

				printf("%u\n", State.ScanCode);
			}break;
			case WM_QUIT: {
				Running = 0;
			}break;
			}
			DispatchMessageA(&CurrentMessage);
		}
		if(NeedsResize){
			RECT NewRect;
			GetClientRect(WindowHandle, &NewRect);
			UINT NewWidth = NewRect.right - NewRect.left;
			UINT NewHeight = NewRect.bottom - NewRect.top;
			Win32ResizeFrameBuffer(&GlobalFrameBuffer, &BackBuffer,NewWidth,NewHeight);
			NeedsResize = 0;
		}

		static int Offset = 0;
		Win32ClearBackBuffer(&GlobalFrameBuffer);
		RenderFrame((IScreenBuffer*) &GlobalFrameBuffer, Offset);
		Offset++;
	
		Win32DisplayBuffer(WindowHandle, GlobalDeviceContext,&GlobalFrameBuffer);
		

		

		


		
	}

}