#include <windows.h>
#include <stdio.h>
#include <math.h>


#define DEBUG 1
#define ArrayCount(x) (sizeof(x)/sizeof((x)[0]))
#define ASSERT(x) if(!(x)) *(char*)0=0;

#define ToBit(x) ((x)*8)

typedef unsigned char byte;


#define global static
#define internal static

global HDC GlobalDeviceContext = NULL;

global bool NeedsResize = 0;

struct ColorBGRA{
	byte B;
	byte G;
	byte R;
	byte A;
};
struct ColorRGBA{
	byte R;
	byte G;
	byte B;
	byte A;
};



struct BackBuffer{
	UINT Width;
	UINT Height;
	ColorBGRA *Data;
	UINT Stride;
	WORD PixelSize;
};

struct FrameBuffer{
	BITMAPINFO BMPInfo;
	UINT Width;
	UINT Height;
	ColorBGRA **Pixels;
	WORD PixelSize;
};

typedef struct KeyDownState{
	unsigned RepeatCount : 16;
	unsigned ScanCode : 8;
	unsigned IsExtended : 1;
	unsigned Reserved : 4;
	unsigned ContextCode : 1;
	unsigned PreviousState : 1;
	unsigned TransitionState : 1;
}KeyDownState;


FrameBuffer GlobalFrameBuffer;

internal void DisplayBuffer(HWND WindowHandle, HDC DeviceContext, FrameBuffer *Buffer){
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

void ResizeFrameBuffer(struct FrameBuffer *FrameBuffer, struct BackBuffer *BackBuffer, UINT NewWidth, UINT NewHeight){
	if((NewWidth > BackBuffer->Width) || (NewHeight > BackBuffer->Height)){
		if(BackBuffer->Data){
			VirtualFree(BackBuffer->Data, 0, MEM_RELEASE);
			BackBuffer->Data = NULL;
		}
		BackBuffer->Width = NewWidth;
		BackBuffer->Height = NewHeight;
		BackBuffer->Data = (ColorBGRA *)VirtualAlloc(NULL, BackBuffer->Width * BackBuffer->Height * BackBuffer->PixelSize, MEM_COMMIT, PAGE_READWRITE);
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
		DisplayBuffer(WindowHandle, DeviceContext,&GlobalFrameBuffer);

		EndPaint(WindowHandle, &PaintStruct);
		
	}break;
	default: {
		Result = DefWindowProcA(WindowHandle, Message, WParam, LParam);
		break;
	}
	
	}

	return Result;
}



internal void RenderFrame(struct FrameBuffer *Buffer,int XOffset){
	ASSERT(Buffer);
	ColorBGRA *Pixels = *(Buffer->Pixels);
	ASSERT(Pixels);
	
	int PosX = XOffset % Buffer->Width;
	int PosY = 20;
	int Radius = 20;

	ColorBGRA Color;
	Color.B = 255;
	Color.G = 0;
	Color.R = 0;
	Color.A = 255;



	for(int x = -Radius; x <= Radius; x++){
		for(int y = -Radius; y <=Radius; y++){
			int DistanceSquare = x * x + y * y;
			if(DistanceSquare < Radius * Radius){
				
				int CoveredPixelX = max(PosX + x, 0);
				int CoveredPixelY = max(PosY + y, 0);
				unsigned Index = CoveredPixelX + CoveredPixelY * Buffer->Width;
				Pixels[Index] = Color;
				ASSERT(Index < Buffer->Height * Buffer->Width);
			}
			
			

		}
	}

	

}

internal void ClearBackBuffer(struct FrameBuffer *Buffer){
	ASSERT(Buffer);
	ASSERT(Buffer->Pixels);
	ASSERT(*(Buffer->Pixels))

	void *Data = *(Buffer->Pixels);
	UINT Size = Buffer->Width * Buffer->Height * (UINT)Buffer->PixelSize;
	memset(Data, 0, Size);
}


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
	
	
	if(!WindowHandle){
#ifdef DEBUG
		
#endif 
		exit(-1);
	}
	
	RECT CurrentRect;
	ASSERT(GetClientRect(WindowHandle, &CurrentRect));
	
	HDC ScreenDeviceContext = GetDC(NULL);
	ASSERT(ScreenDeviceContext);



	BackBuffer BackBuffer = {};
	BackBuffer.Width  = CurrentRect.right - CurrentRect.left;
	BackBuffer.Height = CurrentRect.bottom - CurrentRect.top;
	BackBuffer.PixelSize = sizeof(ColorBGRA);
	BackBuffer.Data = (ColorBGRA*)VirtualAlloc(NULL, BackBuffer.Width * BackBuffer.Height * BackBuffer.PixelSize, MEM_COMMIT, PAGE_READWRITE);
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

	bool Running = 1;

	while (Running) {
		MSG CurrentMessage;

		while (PeekMessageA(&CurrentMessage, 0, 0, 0, PM_REMOVE)) {
			UINT MessageIdentifier = CurrentMessage.message;
			unsigned long WParam = (unsigned long) CurrentMessage.wParam;
			unsigned long LParam = (unsigned long) CurrentMessage.lParam;
			

			switch (MessageIdentifier){
			case WM_KEYDOWN: {
				KeyDownState State;
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
			ResizeFrameBuffer(&GlobalFrameBuffer, &BackBuffer,NewWidth,NewHeight);
			NeedsResize = 0;
		}

		static int Offset = 0;
		ClearBackBuffer(&GlobalFrameBuffer);
		RenderFrame(&GlobalFrameBuffer,Offset);
		Offset++;
	
		DisplayBuffer(WindowHandle, GlobalDeviceContext,&GlobalFrameBuffer);
		

		

		


		
	}

}