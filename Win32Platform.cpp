#define DEBUG 1
#include "Ix64.h"

#include "windowsx.h"
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "IDebug.h"



#define ArrayCount(x) (sizeof(x)/sizeof((x)[0]))
#define ToBit(x) ((x)*8)

//#include "Application.cpp"


struct Win32FrameBuffer{
	unsigned Width;
	unsigned Height;
	unsigned PixelSize;
	unsigned *Pixels;
	BITMAPINFO BMPInfo;
};

struct Win32BackBuffer{
	UINT Width;
	UINT Height;
	unsigned *Data;
	UINT Stride;
	WORD PixelSize;
};

struct Win32KeyDownState{
	unsigned RepeatCount : 16;
	unsigned ScanCode : 8;
	unsigned IsExtended : 1;
	unsigned Reserved : 4;
	unsigned ContextCode : 1;
	unsigned PreviousState : 1;
	unsigned TransitionState : 1;
};




global HDC GlobalDeviceContext = NULL;
global bool NeedsResize = 0;

global Win32FrameBuffer GlobalFrameBuffer;



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
	void *BufferData = Buffer->Pixels;
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
		BackBuffer->Data = (unsigned *)VirtualAlloc(NULL, BackBuffer->Width * BackBuffer->Height * BackBuffer->PixelSize, MEM_COMMIT, PAGE_READWRITE);
		ASSERT(BackBuffer->Data);
	}

	FrameBuffer->Width = NewWidth;
	FrameBuffer->Height = NewHeight;
	FrameBuffer->BMPInfo.bmiHeader.biWidth = FrameBuffer->Width;
	FrameBuffer->BMPInfo.bmiHeader.biHeight = -(INT)FrameBuffer->Height;
	FrameBuffer->Pixels = BackBuffer->Data;
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
	ASSERT(Buffer->Pixels);

	void *Data = Buffer->Pixels;
	UINT Size = Buffer->Width * Buffer->Height * (UINT)Buffer->PixelSize;
	memset(Data, 0, Size);
}

internal unsigned long long GetLastFileWriteTime(char *Path){
	WIN32_FILE_ATTRIBUTE_DATA FileData = {0};
	if(GetFileAttributesExA(Path, GetFileExInfoStandard, &FileData)){
		return ((unsigned long long)FileData.ftLastWriteTime.dwHighDateTime) << 32 | ((unsigned long long)FileData.ftLastWriteTime.dwLowDateTime);
	}
	return 0;
}



internal void ReloadDLL(char *DLLPath, char *DLLName, HMODULE *DLL, render_frame** proc, unsigned long long *FileTime) {
	if(*DLL){
		ASSERT(FreeLibrary(*DLL));
	}
	unsigned long long NewFileTime = GetLastFileWriteTime(DLLPath);
	ASSERT(NewFileTime > *FileTime);

	int i = 0; 
	//DLL might still be locked by linker. Try 1000 times or crash
	while(!CopyFileA(DLLPath, "x64/Debug/tmp_Application.dll", 0)){
		i++;
		if(i > 1000){
			DWORD Error = GetLastError();
			ASSERT(0);
		}
	}

	*DLL = LoadLibraryA("tmp_Application.dll");
	ASSERT(DLL);
	*proc = (render_frame *)GetProcAddress(*DLL, "RenderFrame");
	ASSERT(*proc);
	*FileTime = NewFileTime;
}

#ifdef DEBUG
	#include "DebugWin32.cpp"
#endif
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	long long PerfCountFrequency;
	QueryPerformanceFrequency((LARGE_INTEGER *)&PerfCountFrequency);
	

	HMODULE DLL = 0;
	char *DLLPath = (char *)"x64/Debug/Application.dll";
	render_frame *RenderFrame = 0;
	unsigned long long DLLWriteTime = 0;
	ReloadDLL(DLLPath, (char *)"Application.dll", &DLL, &RenderFrame, &DLLWriteTime);

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

	if(!WindowHandle){
		exit(-1);
	}
	
	RECT CurrentRect;
	ASSERT(GetClientRect(WindowHandle, &CurrentRect));
	
	HDC ScreenDeviceContext = GetDC(NULL);
	ASSERT(ScreenDeviceContext);



	Win32BackBuffer BackBuffer = {};
	BackBuffer.Width  = CurrentRect.right - CurrentRect.left;
	BackBuffer.Height = CurrentRect.bottom - CurrentRect.top;
	BackBuffer.PixelSize = sizeof(unsigned);
	BackBuffer.Data = (unsigned*)VirtualAlloc(NULL, BackBuffer.Width * BackBuffer.Height * BackBuffer.PixelSize, MEM_COMMIT, PAGE_READWRITE);
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
	GlobalFrameBuffer.Pixels = BackBuffer.Data;

#ifdef DEBUG
	GlobalDebugCTX.DebugWindowHandle = WindowHandle;
	GlobalDebugCTX.DebugDeviceContext = GlobalDeviceContext;
	GlobalDebugCTX.DebugFrameBuffer = &GlobalFrameBuffer;

	IDebugContext AppDebugCTX = {0};
	AppDebugCTX.DebugDisplayBuffer = DebugDisplayBuffer;
#endif

	bool PrintFrameTime;
	bool Running = 1;
	long long Previous, Current;
	QueryPerformanceCounter((LARGE_INTEGER *)&Previous);
	while (Running) {

		if(DLLWriteTime != GetLastFileWriteTime(DLLPath)){
			ReloadDLL(DLLPath,(char*)"Application.dll",&DLL,&RenderFrame,&DLLWriteTime);
		}


		MSG CurrentMessage;
		IInput Input = {};
		while (PeekMessageA(&CurrentMessage, 0, 0, 0, PM_REMOVE)) {
			UINT MessageIdentifier = CurrentMessage.message;
			unsigned long WParam = (unsigned long) CurrentMessage.wParam;
			unsigned long LParam = (unsigned long) CurrentMessage.lParam;
			

			switch (MessageIdentifier){
			case WM_KEYDOWN: {
				Win32KeyDownState State;
				ASSERT(sizeof(State) == sizeof(LParam));
				State = *(Win32KeyDownState *)&LParam;

				printf("%u\n", State.ScanCode);
			}break;
			case WM_MOUSEMOVE: {
				Input.LeftClick	  = (WParam & MK_LBUTTON) != 0;
				Input.RightClick  = (WParam & MK_RBUTTON) != 0;
				Input.MiddleClick = (WParam & MK_MBUTTON) != 0;
				
				Input.MouseX = GET_X_LPARAM(LParam);
				Input.MouseY = GET_Y_LPARAM(LParam);
				//printf("LeftClick: %d, MouseX: %hd, MouseY: %hd\n", Input.LeftClick, Input.MouseX, Input.MouseY);
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
		RenderFrame(&AppDebugCTX,(IScreenBuffer*) &GlobalFrameBuffer, Offset,&Input,&PrintFrameTime);

		Offset++;
		
		Win32DisplayBuffer(WindowHandle, GlobalDeviceContext,&GlobalFrameBuffer);
		

		QueryPerformanceCounter((LARGE_INTEGER *)&Current);
		long long TicksElapsed = Current - Previous;
		//Convert To milliseconds
		float CounterElapsed = (float)(TicksElapsed * 1000) / (float)PerfCountFrequency;
		
		if(PrintFrameTime){
			printf("RenderFrame:%f ms\n", CounterElapsed);

		}
		Previous = Current;
	}

}